/*
    Copyright (c) 2004-2005, Jeremy Cole and others

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <errno.h>
#include <assert.h>

#define _GNU_SOURCE
#include <getopt.h>

#include <mygis/mygis.h>
#include <mygis/pairlist.h>
#include <mygis/geometry.h>
#include <mygis/shapefile.h>
#include <mygis/wkt/wkt.h>

#include <mygis/dbug.h>

const char *program = {"mysqlgisimport"};
const char *version = {"0.5"};

static const char *short_options = {"?dDXPSsnt:q:g:o:r:a:p:Gi"};
static struct option long_options[] = {
  {"help",                no_argument,       NULL, '?'},
#ifdef DEBUG
  {"debug",               optional_argument, NULL, 'd'},
#endif
  {"no-dbf",              no_argument,       NULL, 'D'},
  {"no-shp",              no_argument,       NULL, 'S'},
  {"no-shx",              no_argument,       NULL, 'X'},
  {"no-prj",              no_argument,       NULL, 'P'},
  {"no-schema",           no_argument,       NULL, 's'},
  {"no-data",             no_argument,       NULL, 'n'},
  {"table",               required_argument, NULL, 't'},
  {"query",               required_argument, NULL, 'q'},
  {"geometry-field",      required_argument, NULL, 'g'},
  {"output",              required_argument, NULL, 'o'},
  {"remap",               required_argument, NULL, 'r'},
  {"auto-increment-key",  required_argument, NULL, 'a'},
  {"primary-key",         required_argument, NULL, 'p'},
  {"geometry-as-text",    no_argument,       NULL, 'G'},
  {"delimited",           no_argument,       NULL, 'i'},
  {0, 0, 0, 0}
};

void usage(FILE *f)
{
  DBUG_ENTER("usage");
  fprintf(f, "\n");
  fprintf(f, "%s %s, %s\n", program, version, mygis_version);
  fprintf(f, "%s\n", mygis_copyright);
  fprintf(f, "Distributed under the %s\n", mygis_license);
  fprintf(f, "\n");
  fprintf(f, "Usage: %s [options] <shapefile> [ <shapefile> ... ]\n", program);

  fprintf(f, "\nGeneral Options:\n");
  fprintf(f, "  -?, --help           Display this help and exit.\n");
#ifdef DEBUG
  fprintf(f, "  -d, --debug          Output debugging information while running.\n");
#endif

  fprintf(f, "\nInput Options:\n");
  fprintf(f, "  -D, --no-dbf         Don't use a DBF (database) file.\n");
  fprintf(f, "  -S, --no-shp         Don't use a SHP (shape) file, implies --no-shx.\n");
  fprintf(f, "  -X, --no-shx         Don't use a SHX (shape index) file.\n");
  fprintf(f, "  -P, --no-prj         Don't use a PRJ (projection) file.\n");
  
  fprintf(f, "\nFilter Options:\n");
  fprintf(f, "  -q, --query          DBF query of form: \"FIELD=value\".\n");
  
  fprintf(f, "\nDatabase Options:\n");
  fprintf(f, "  -t, --table          Table name to load records into.\n");
  fprintf(f, "  -r, --remap          Remap a DBF column to a another name.\n");
  fprintf(f, "                       Expects `dbf_name=new_name' as an argument.\n");
  fprintf(f, "  -a, --auto-increment-key\n"
             "                       Name of auto-incremented primary key field (default `id').\n");
  fprintf(f, "  -p, --primary-key    Name (after remap) of existing field to use as our primary key.\n");
  fprintf(f, "  -g, --geometry-field Name of GEOMETRY field, default `geo` or `geo_as_text`.\n");
  
  fprintf(f, "\nOutput Options:\n");
  fprintf(f, "  -o, --output         Output to a file, defaults to stdout.\n");
  fprintf(f, "  -s, --no-schema      Don't output any schema, only the data.\n");
  fprintf(f, "  -n, --no-data        Don't output any data, only the schema.\n");
  fprintf(f, "  -i, --delimited      Output data in delimited format.\n"
             "                       Implies --no-schema.\n");
  fprintf(f, "  -G, --geometry-as-text\n"
             "                       Output geometry as text rather than GEOMETRY.\n");

  fprintf(f, "\n");
  DBUG_VOID_RETURN;
}

char *sql_table_name(char *name)
{
  static char sql[200];

  DBUG_ENTER("sql_table_name");
  sprintf(sql, "`%s`", name);
  DBUG_RETURN(sql);
}


char *sql_backquote(char * field_name)
{
  static char sql[200];

  DBUG_ENTER("sql_backquote");
  /* TODO: Need to do some cleanup of the field name. */
  sprintf(sql, "`%s`", field_name);
  DBUG_RETURN(sql);
}


char *sql_field_name(DBF_FIELD *field, PAIRLIST *remap)
{
  DBUG_ENTER("sql_field_name");
  DBUG_RETURN(sql_backquote(pairlist_get_value(remap, field->name)));
}


char *sql_field_type(DBF_FIELD *field)
{
  static char sql[200];

  DBUG_ENTER("sql_field_type");

  if(!field) DBUG_RETURN(NULL);

  switch(field->type) {
  case DBF_CHARACTER:
    sprintf(sql, "CHAR(%i) NOT NULL", field->length);
    break;
  case DBF_DATE:
    sprintf(sql, "DATE NOT NULL");
    break;
  case DBF_NUMBER:
    if (field->decimals)
      sprintf(sql, "DOUBLE(%i,%i) NOT NULL", field->length, field->decimals);
    else
      if (field->length <= 9)
        sprintf(sql, "INT(%i) NOT NULL", field->length);
      else
        sprintf(sql, "BIGINT(%i) NOT NULL", field->length);
    break;
  case DBF_FLOATING:
    sprintf(sql, "DOUBLE(%i,%i) NOT NULL", field->length, field->decimals);
    break;
  case DBF_LOGICAL:
    sprintf(sql, "CHAR(1) NOT NULL");
    break;
  }
  DBUG_RETURN(sql);
}

/*
  print str to f as a single quoted string. Single quotes
  will be escaped as ''. For instance, "don't" becomes
  "'don''t'"
*/
void print_single_quoted_string(FILE *f, char *str)
{
  int i;

  DBUG_ENTER("print_single_quoted_string");

  fprintf(f, "'");

  /* print characters in the string one by one */
  for (i = 0; str[i] != '\0'; i++)
    {
      if (str[i] == '\'')
        fprintf(f, "''");
      else
        fputc(str[i], f);
    }
  
  fprintf(f, "'");

  DBUG_VOID_RETURN;
}

/*
  Print str as part of a delimited record (for the LOAD DATA INFILE
  command). Escapes tab as \t, newline as \n, and backslash as \\
*/
void print_delimited_string(FILE *f, char *str)
{
  int i;
 
  DBUG_ENTER("print_delimited_string");

  /* print characters in the string one by one */
  for (i = 0; str[i] != '\0'; i++)
    {
      if (str[i] == '\\')
        fprintf(f, "\\\\");

      else if (str[i] == '\t')
        fprintf(f, "\\t");

      else if (str[i] == '\n')
        fprintf(f, "\\n");

      else
        fputc(str[i], f);
     }
  
  DBUG_VOID_RETURN;
}



void print_sql_cell(FILE *f, CELL *cell, int opt_delimited)
{
  DBUG_ENTER("print_sql_cell");

  /* TODO: Need to escape the data for SQL. */

  assert(cell);

  switch(cell->metadata->data_type) {
  case CHARACTER:
    if (opt_delimited)
      print_delimited_string(f, cell->data.character);
    else
      print_single_quoted_string(f, cell->data.character);
    break;

  case DATE:
    if (opt_delimited)
      print_delimited_string(f, cell->data.date);
    else
      print_single_quoted_string(f, cell->data.date);
    break;

  case NUMBER:
    fprintf(f, cell->metadata->format, cell->data.number);
    break;

  case FLOATING:
    fprintf(f, cell->metadata->format, cell->data.floating);
    break;

  case LOGICAL:
    if (opt_delimited)
      fprintf(f, "%c", cell->data.logical);
    else
      fprintf(f, "'%c'", cell->data.logical);
    break;
  }

  DBUG_VOID_RETURN;
}

void print_schema(FILE *f, SHAPEFILE *sha, 
                  char *table_name, char *geometry_field,
                  PAIRLIST *remap,
                  char *auto_increment_key, char *primary_key,
                  int opt_geometry_as_text)
{
  DBF *dbf = sha->dbf;
  DBF_FIELD *field;
  int i;

  char* primary_or_auto_increment_key = (auto_increment_key ? auto_increment_key : primary_key);

  DBUG_ENTER("print_schema");

  fprintf(f, "DROP TABLE IF EXISTS `%s`;\n", table_name);
  fprintf(f, "CREATE TABLE `%s` (\n", table_name);
  
  if (auto_increment_key)
    fprintf(f, "  %-20s INT UNSIGNED NOT NULL auto_increment,\n", sql_backquote(auto_increment_key));


  if(sha->flags & SHAPEFILE_HAS_DBF) {
    FOREACH_DBF_FIELD(dbf, field, i) {
      fprintf(f, "  %-20s %s,\n", 
              sql_field_name(field, remap), 
              sql_field_type(field));
    }
  }

  if(sha->flags & SHAPEFILE_HAS_SHP) {
    if (opt_geometry_as_text)
      {
        fprintf(f, "  %-20s MEDIUMTEXT NOT NULL,\n",
                sql_backquote(geometry_field));
      } else {
        fprintf(f, "  %-20s GEOMETRY NOT NULL,\n",
                sql_backquote(geometry_field));
        fprintf(f, "  SPATIAL INDEX (%s),\n",
                sql_backquote(geometry_field));
      }
  }

  fprintf(f, "  PRIMARY KEY (%s)\n",
          sql_backquote(primary_or_auto_increment_key));

  fprintf(f, ");\n\n");

  DBUG_VOID_RETURN;
}

void print_record(FILE *f,
                  SHAPEFILE_RECORD *record, 
                  char *table_name,
                  char *auto_increment_key,
                  int opt_delimited,
                  int opt_geometry_as_text)
{
  SHAPEFILE *sha = record->shapefile;
  RECORD *dbf_record = record->dbf_record;
  DBF_FIELD *field;
  CELL *cell;
  CELL_NODE *cell_node = dbf_record->head;
  int i;
  char *separator;

  DBUG_ENTER("print_record");

  if (opt_delimited)
    separator = "\t";
  else
    separator = ", ";

  if (!opt_delimited)
    fprintf(f, "INSERT INTO `%s` VALUES (", table_name);

  /* dummy null value to fill in the auto-incremented field */
  if (auto_increment_key) {
    if (opt_delimited)
      fprintf(f, "\\N");
    else
      fprintf(f, "NULL");
  }
  
  if(sha->flags & SHAPEFILE_HAS_DBF) {
    for(; cell_node; cell_node = cell_node->next) {
      cell = cell_node->cell;
      field = (DBF_FIELD *)cell->field;

      /* don't print a separator before the first record */
      if (auto_increment_key || i != 0)
        fprintf(f, "%s", separator);

      print_sql_cell(f, cell, opt_delimited);
    }
  }

  if(sha->flags & SHAPEFILE_HAS_SHP) {
    fprintf(f, "%s", separator);

    /* convert to a MySQL geometry object */
    if (!opt_geometry_as_text)
      fprintf(f, "GEOMFROMTEXT(");

    if (!opt_delimited)
      fprintf(f, "'");

    wkt_write(record->geometry, sha->projection, f);
    
    if (!opt_delimited)
      fprintf(f, "'");

    if (!opt_geometry_as_text)    
      fprintf(f, ")");
  }
  
  if (!opt_delimited)
    fprintf(f, "\n);");
  
  fprintf(f, "\n");

  DBUG_VOID_RETURN;
}

SHAPEFILE_SCAN *scan_query(SHAPEFILE *sha, char *query)
{
  SHAPEFILE_SCAN *scan = NULL;
  char *key, *value, *p;

  DBUG_ENTER("scan_query");

  if((p=index(query, '='))) {
    key = query;
    *p = '\0';
    value = ++p;
    scan = shapefile_scan_init(sha, &compare_string_ci_eq, key, value);
  } else {
    fprintf(stderr, "Bad query format '%s', should be of the form 'key=value'.", query);
    DBUG_RETURN(NULL);
  }
  DBUG_RETURN(scan);
}

int main(int argc, char **argv)
{
  SHAPEFILE *sha;
  SHAPEFILE_SCAN *scan;
  SHAPEFILE_RECORD *rec;
  PROJECTION *proj;
  int shapefile_flags = 0;
  int ret = 0;

  FILE *output = stdout;

  PAIRLIST *remap = NULL;
  DBF_FIELD *field;
  int i;
  int filename_arg_index;

  int opt, option_index;
  int opt_no_schema        = 0;
  int opt_no_data          = 0;
  int opt_geometry_as_text = 0;
  int opt_delimited    = 0;

  char *table_name         = NULL;
  char *geometry_field     = NULL;
  char *auto_increment_key = NULL;
  char *primary_key        = NULL;

  char *query = NULL;
  char *ptr   = NULL;

  DBUG_ENTER("main");
  DBUG_PROCESS(argv[0]);

  if(!(remap = pairlist_init(&compare_string_ci_eq, &compare_string_ci_eq)))
    DBUG_RETURN(1);

  while(1) {
    opt = getopt_long(argc, argv, short_options, long_options, &option_index);
    if (opt == -1) break;

    switch(opt) {
    case '?':
      usage(stdout);
      DBUG_RETURN(0);
    case 'd':
      DBUG_PUSH(optarg?optarg:"d:t");
      break;
    case 'D':
      shapefile_flags |= SHAPEFILE_NO_DBF;
      break;
    case 'S':
      shapefile_flags |= SHAPEFILE_NO_SHP|SHAPEFILE_NO_SHX;
      break;
    case 'X':
      shapefile_flags |= SHAPEFILE_NO_SHX;
      break;
    case 'P':
      shapefile_flags |= SHAPEFILE_NO_PRJ;
      break;
    case 's':
      opt_no_schema++;
      break;
    case 'n':
      opt_no_data++;
      break;
    case 't':
      table_name = (char *)strdup(optarg);
      break;
    case 'q':
      query = (char *)strdup(optarg);
      break;
    case 'g':
      geometry_field = (char *)strdup(optarg);
      break;
    case 'o':
      if(optarg[0] == '-' && optarg[1] == '\0') break;
      if(!(output = fopen(optarg, "w"))) {
        fprintf(stderr, "Couldn't open output file `%s': Error %i: %s\n",
	        optarg, errno, strerror(errno));
        goto err1;
      break;
    case 'r':
      if(!(ptr=strchr(optarg, '=')))
        goto err1;
      *ptr++ = '\0';
      pairlist_add(remap, optarg, ptr); 
      break;
    case 'a':
      auto_increment_key = (char *)strdup(optarg);
      break;
    case 'p':
      primary_key = (char *)strdup(optarg);
      break;
    case 'G':
      opt_geometry_as_text++;
      break;
    case 'i':
      opt_delimited++;
      opt_no_schema++; /* can't output schema along with delimited format */
      break;
      }
    }
  }
  
  if(primary_key && auto_increment_key) {
    fprintf(stderr, "You can't specify both --auto-increment-key and --primary-key at the same time.  \n");
    goto err1;
  }

  /* validate arguments */

  /* if there aren't any files specified */
  if(optind == argc) {
    usage(stderr);
    ret = 1; goto err1;
  }

  if((shapefile_flags & SHAPEFILE_NO_DBF) &&
     (shapefile_flags & SHAPEFILE_NO_SHP)) {
    fprintf(stderr, "ERROR: You must have either a .shp or .dbf file.\n");
    ret = 1; goto err1;
  }

  if (opt_delimited && !opt_geometry_as_text) {
    fprintf(stderr, "ERROR: You must specify -G with -i \n(can't yet output raw MySQL GEOMETRY records)\n");
    ret = 1; goto err1;
  }

  if (auto_increment_key && primary_key) {
    fprintf(stderr, "ERROR: You may specify -a or -p, but not both \n(can only have one primary key)\n");
    ret = 1; goto err1;
  }

  /* set defaults */

  if (geometry_field == NULL)
  {
    geometry_field = (opt_geometry_as_text ? "geo_as_text" : "geo");
  }

  if (!auto_increment_key && !primary_key)
  {
    auto_increment_key = "id";
  }

  proj = projection_init();

  /* open each shapefile and start processing */
  
  for (filename_arg_index = optind; 
       filename_arg_index < argc; 
       filename_arg_index++)
    {

      if(!(sha = shapefile_init(shapefile_flags))) {
        fprintf(stderr, "Couldn't initialize, out of memory?\n");
        ret = 2; goto err1;
      }

      if(shapefile_open(sha, argv[filename_arg_index], 'r') < 0) {
        fprintf(stderr, "Couldn't open files, missing files?\n");
        ret = 3; goto err2;
      }
      
      if(sha->flags & SHAPEFILE_HAS_PRJ)
      {
        projection_set(proj, sha->prj->proj4_def, "+proj=latlong");
        shapefile_set_projection(sha, proj);
      }

      if(!table_name) table_name = (char *)strdup(basename(argv[optind]));

      if(sha->flags & SHAPEFILE_HAS_DBF) {
        FOREACH_DBF_FIELD(sha->dbf, field, i) {
          if(!pairlist_get(remap, field->name))
            pairlist_add(remap, field->name, field->name);
        }
      }

      /* if this is the first file, print out
         a CREATE TABLE statement */
      if(filename_arg_index == optind && !opt_no_schema)
        print_schema(output, sha, table_name, geometry_field, remap, 
                     auto_increment_key, primary_key, opt_geometry_as_text);

      if(!opt_no_data) {
        if(query) {
          if(!(scan = scan_query(sha, query))) goto err3;
        } else {
          scan = shapefile_scan_init(sha, NULL, NULL, NULL);
        }
        while((rec = shapefile_scan_read_next(scan))) {
          print_record(output, rec, table_name, auto_increment_key,
                       opt_delimited, opt_geometry_as_text);
          shapefile_record_free(rec);
        } 
        shapefile_scan_free(scan);
      }

    }

 err3:
  shapefile_close(sha);
 err2:
  shapefile_free(sha);
 err1:
  pairlist_free(remap);
  DBUG_RETURN(ret);
}
