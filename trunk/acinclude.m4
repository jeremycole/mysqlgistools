m4_include([libtool.m4])

# System libraries

AC_DEFUN([AC_LIB_Z_FOR_MYSQL], [
  if test "x$want_mysql" = "xyes"
  then
    AC_CHECK_LIB(z, compress)
  fi
])

AC_DEFUN([AC_LIB_MYGIS], [
  AC_CHECK_LIB(mygis, shapefile_open)
])

AC_DEFUN([AC_FUNC_CRYPT], [
  AC_CHECK_LIB(crypt, crypt)
  AC_CHECK_FUNC(crypt, AC_DEFINE([HAVE_CRYPT], [], [the `crypt' library]))
])

AC_DEFUN([AC_LIB_PROJ], [
  if test "x$want_projection" = "xyes"
  then
    AC_CHECK_LIB(proj, pj_init)
  fi
])

# MySQL Libraries and Headers

AC_DEFUN([AC_LIB_MYSQL], [
  AC_ARG_WITH(mysql-lib,
  [  --with-mysql-lib=DIR    Look for MySQL client library in DIR],
  mysql_lib=$withval, mysql_lib="")

  if test "x$want_mysql" = "xyes"
  then
    AC_MSG_CHECKING([for libmysqlclient])
    AC_MSG_RESULT([])

    mysql_ok=no
    mysql_found=no

    SAVE_LIBS=$LIBS
    
    mysql_lib="$mysql_lib /usr/lib /usr/lib/mysql \
                /usr/local/lib /usr/local/lib/mysql \
                /usr/local/mysql/lib"
    
    for dir in $mysql_lib; do
      if test "x$mysql_found" != "xyes"
      then
        AC_MSG_CHECKING([for libmysqlclient in $dir])
        if test -f "$dir/libmysqlclient.a" ;
        then
          AC_MSG_RESULT([yes])
          LIBS="-L$dir $SAVE_LIBS $LIBZ_LIB"
          MYSQL_LIB="-L$dir -lmysqlclient $LIBZ_LIB"
          AC_SUBST(MYSQL_LIB)
          AC_CHECK_LIB(mysqlclient, mysql_real_connect,
                       [mysql_ok=yes; mysql_found=yes], mysql_ok=no)
        else
          AC_MSG_RESULT([no])
        fi
      fi
    done

    if test "x$mysql_ok" != "xyes"
    then
      AC_MSG_ERROR([Could not find libmysqlclient in '$mysql_lib'])
    fi
  fi
])


AC_DEFUN([AC_HEADER_MYSQL], [
  AC_ARG_WITH(mysql-include,
  [  --with-mysql-include=DIR
                          Look for MySQL include files in DIR],
  mysql_include=$withval, mysql_include="")

  if test "x$want_mysql" = "xyes"
  then
    AC_MSG_CHECKING([for mysql.h])
    AC_MSG_RESULT()

    mysql_found=no

    mysql_include="$mysql_include /usr/include /usr/include/mysql \
                    /usr/local/include /usr/local/include/mysql \
                    /usr/local/mysql/include"
    
    for dir in $mysql_include; do
      if test "x$mysql_found" != "xyes"
      then
        AC_MSG_CHECKING([for mysql.h in $dir])
        if test -f "$dir/mysql.h" 
        then
          MYSQL_INCLUDE="-I$dir"
          AC_SUBST(MYSQL_INCLUDE)
          mysql_found=yes
          AC_MSG_RESULT([yes])
        else
          AC_MSG_RESULT([no])
        fi
      fi
    done

    if test "x$mysql_found" != "xyes"
    then
      AC_MSG_ERROR([Could not find mysql.h in '$mysql_include'])
    fi
  fi
])
