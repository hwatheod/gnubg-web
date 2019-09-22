/*
 * dbprovider.c
 *
 * by Jon Kinsey, 2008
 *
 * Database providers
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 3 or later of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: dbprovider.c,v 1.49 2015/04/14 22:17:15 plm Exp $
 */

#include "config.h"
#include "gnubgmodule.h"
#include "stdlib.h"

#include "backgammon.h"
#include <glib/gstdio.h>
#include <string.h>
#include "dbprovider.h"

DBProviderType dbProviderType = (DBProviderType) 0;
int storeGameStats = TRUE;

#if USE_PYTHON
#include "pylocdefs.h"

static PyObject *pdict;
RowSet *ConvertPythonToRowset(PyObject * v);

#if !USE_SQLITE
static int PySQLiteConnect(const char *dbfilename, const char *user, const char *password, const char *hostname);
#endif
static int PyMySQLConnect(const char *dbfilename, const char *user, const char *password, const char *hostname);
static void PyDisconnect(void);
static RowSet *PySelect(const char *str);
static int PyUpdateCommand(const char *str);
static void PyCommit(void);
#if !defined(WIN32)
static int PyPostgreConnect(const char *dbfilename, const char *user, const char *password, const char *hostname);
static GList *PyPostgreGetDatabaseList(const char *user, const char *password, const char *hostname);
static int PyPostgreDeleteDatabase(const char *dbfilename, const char *user, const char *password,
                                   const char *hostname);
#endif
static GList *PyMySQLGetDatabaseList(const char *user, const char *password, const char *hostname);
static int PyMySQLDeleteDatabase(const char *dbfilename, const char *user, const char *password, const char *hostname);
#endif
#if USE_SQLITE
static int SQLiteConnect(const char *dbfilename, const char *user, const char *password, const char *hostname);
static void SQLiteDisconnect(void);
static RowSet *SQLiteSelect(const char *str);
static int SQLiteUpdateCommand(const char *str);
static void SQLiteCommit(void);
#endif

#if NUM_PROVIDERS
static int SQLiteDeleteDatabase(const char *dbfilename, const char *user, const char *password, const char *hostname);
static GList *SQLiteGetDatabaseList(const char *user, const char *password, const char *hostname);
static DBProvider providers[NUM_PROVIDERS] = {
#if USE_SQLITE
    {SQLiteConnect, SQLiteDisconnect, SQLiteSelect, SQLiteUpdateCommand, SQLiteCommit, SQLiteGetDatabaseList,
     SQLiteDeleteDatabase,
     "SQLite", "SQLite", "Direct SQLite3 connection", FALSE, TRUE, "gnubg", "", "", ""},
#endif
#if USE_PYTHON
#if !USE_SQLITE
    {PySQLiteConnect, PyDisconnect, PySelect, PyUpdateCommand, PyCommit, SQLiteGetDatabaseList, SQLiteDeleteDatabase,
     "SQLite (Python)", "PythonSQLite", "SQLite3 connection included in latest Python version", FALSE, TRUE, "gnubg",
     "", "", ""},
#endif
    {PyMySQLConnect, PyDisconnect, PySelect, PyUpdateCommand, PyCommit, PyMySQLGetDatabaseList, PyMySQLDeleteDatabase,
     "MySQL (Python)", "PythonMySQL", "MySQL connection via MySQLdb Python module", TRUE, TRUE, "gnubg", "", "",
     "localhost:3306"},
#if !defined(WIN32)
    {PyPostgreConnect, PyDisconnect, PySelect, PyUpdateCommand, PyCommit, PyPostgreGetDatabaseList,
     PyPostgreDeleteDatabase,
     "Postgres (Python)", "PythonPostgre", "PostgreSQL connection via PyGreSQL Python module", TRUE, TRUE, "gnubg", "",
     "", "localhost:5432"},
#endif
#endif
};

#else
DBProvider providers[1] = { {0, 0, 0, 0, 0, 0, 0, "No Providers", "No Providers", "No Providers", 0, 0, 0, 0, 0, 0} };
#endif

#if USE_PYTHON || USE_SQLITE
static RowSet *
MallocRowset(size_t rows, size_t cols)
{
    size_t i;
    RowSet *pRow = malloc(sizeof(RowSet));
    g_assert(pRow);

    pRow->widths = (size_t *) malloc(cols * sizeof(size_t));
    memset(pRow->widths, 0, cols * sizeof(size_t));

    pRow->data = malloc(rows * sizeof(char **));
    for (i = 0; i < rows; i++) {
        pRow->data[i] = malloc(cols * sizeof(char *));
        memset(pRow->data[i], 0, cols * sizeof(char *));
    }

    pRow->cols = cols;
    pRow->rows = rows;

    return pRow;
}

static void
SetRowsetData( /*lint -e{818} */ RowSet * rs, size_t row, size_t col, const char *data)
{
    size_t size;
    if (!data)
        data = "";

    rs->data[row][col] = malloc(strlen(data) + 1);
    strcpy(rs->data[row][col], data);

    size = strlen(data);
    if (row == 0 || size > rs->widths[col])
        rs->widths[col] = size;
}
#endif

extern void
FreeRowset(RowSet * pRow)
{
    unsigned int i, j;
    free(pRow->widths);

    for (i = 0; i < pRow->rows; i++) {
        for (j = 0; j < pRow->cols; j++) {
            free(pRow->data[i][j]);
        }
        free(pRow->data[i]);
    }
    free(pRow->data);

    pRow->cols = pRow->rows = 0;
    pRow->data = NULL;
    pRow->widths = NULL;
}

int
RunQueryValue(const DBProvider * pdb, const char *query)
{
    RowSet *rs;
    rs = pdb->Select(query);
    if (rs && rs->rows > 1) {
        int id = (int)strtol(rs->data[1][0], NULL, 0);
        FreeRowset(rs);
        return id;
    } else
        return -1;
}

extern RowSet *
RunQuery(const char *sz)
{
    DBProvider *pdb;
    if ((pdb = ConnectToDB(dbProviderType)) != NULL) {
        RowSet *rs = pdb->Select(sz);
        pdb->Disconnect();
        return rs;
    }
    return NULL;
}

const char *
GetProviderName(int i)
{
    return providers[i].name;
}

static DBProviderType
GetTypeFromName(const char *name)
{
    int i;
    for (i = 0; i < NUM_PROVIDERS; i++) {
        if (!StrCaseCmp(providers[i].shortname, name))
            break;
    }
    if (i == NUM_PROVIDERS)
        return INVALID_PROVIDER;

    return (DBProviderType) i;
}

void
SetDBParam(const char *db, const char *key, const char *value)
{
    DBProviderType type = GetTypeFromName(db);
    if (type == INVALID_PROVIDER)
        return;

    if (!StrCaseCmp(key, "database"))
        providers[type].database = g_strdup(value);
    else if (!StrCaseCmp(key, "username"))
        providers[type].username = g_strdup(value);
    else if (!StrCaseCmp(key, "password"))
        providers[type].password = g_strdup(value);
    else if (!StrCaseCmp(key, "hostname"))
        providers[type].hostname = g_strdup(value);
}

void
SetDBType(const char *type)
{
    dbProviderType = GetTypeFromName(type);
}

void
SetDBSettings(DBProviderType dbType, const char *database, const char *user, const char *password, const char *hostname)
{
    dbProviderType = dbType;
    providers[dbProviderType].database = g_strdup(database);
    providers[dbProviderType].username = g_strdup(user);
    providers[dbProviderType].password = g_strdup(password);
    providers[dbProviderType].hostname = g_strdup(hostname);
}

void
RelationalSaveSettings(FILE * pf)
{
    int i;
    fprintf(pf, "relational setup storegamestats=%s\n", storeGameStats ? "yes" : "no");

    if (dbProviderType != INVALID_PROVIDER)
        fprintf(pf, "relational setup dbtype=%s\n", providers[dbProviderType].shortname);
    for (i = 0; i < NUM_PROVIDERS; i++) {
        DBProvider *pdb = GetDBProvider((DBProviderType) i);
        fprintf(pf, "relational setup %s-database=%s\n", providers[i].shortname, pdb->database);
        fprintf(pf, "relational setup %s-username=%s\n", providers[i].shortname, pdb->username);
        fprintf(pf, "relational setup %s-password=%s\n", providers[i].shortname, pdb->password);
        fprintf(pf, "relational setup %s-hostname=%s\n", providers[i].shortname, pdb->hostname);
    }
}

extern DBProvider *
GetDBProvider(DBProviderType dbType)
{
#if USE_PYTHON
    static int setup = FALSE;
    if (!setup) {
        if (LoadPythonFile("database.py", FALSE) == 0) {
            PyObject *m;
            /* Get main python dictionary */
            if ((m = PyImport_AddModule("__main__")) == NULL) {
                outputl(_("Error importing 'main' module"));
            } else {
                pdict = PyModule_GetDict(m);
                setup = TRUE;
            }
        }
        if (!setup)
            return NULL;
    }
#endif
#if !NUM_PROVIDERS
    (void) dbType;              /* suppress unused parameter compiler warning */
    return NULL;
#else
    if (dbType == INVALID_PROVIDER)
        return NULL;

    return &providers[dbType];
#endif
}

#if USE_PYTHON
int
PyMySQLConnect(const char *dbfilename, const char *user, const char *password, const char *hostname)
{
    long iret;
    PyObject *ret;

    const char *host = hostname ? hostname : "";

    char *buf = g_strdup_printf("PyMySQLConnect(r'%s', '%s', '%s', '%s')", dbfilename, user, password, host);
    /* Connect to database */
    ret = PyRun_String(buf, Py_eval_input, pdict, pdict);
    g_free(buf);
    if (ret == NULL || !PyInt_Check(ret) || (iret = PyInt_AsLong(ret)) < 0) {
        PyErr_Print();
        return -1;
    } else if (iret == 0L) {     /* New database - populate */
        return 0;
    }
    return 1;
}

#if !defined(WIN32)
int
PyPostgreConnect(const char *dbfilename, const char *user, const char *password, const char *hostname)
{
    long iret;
    PyObject *ret;
    const char *host = hostname ? hostname : "";

    char *buf = g_strdup_printf("PyPostgreConnect(r'%s', '%s', '%s', '%s')", dbfilename, user, password, host);
    /* Connect to database */
    ret = PyRun_String(buf, Py_eval_input, pdict, pdict);
    g_free(buf);
    if (ret == NULL || !PyInt_Check(ret) || (iret = PyInt_AsLong(ret)) < 0) {
        PyErr_Print();
        return -1;
    } else if (iret == 0L) {     /* New database - populate */
        return 0;
    }
    return 1;
}
#endif

#if !USE_SQLITE
static int
PySQLiteConnect(const char *dbfilename, const char *UNUSED(user), const char *UNUSED(password),
                const char *UNUSED(hostname))
{
    PyObject *con;
    char *name, *filename, *buf;
    int exists;

    name = g_strdup_printf("%s.db", dbfilename);
    filename = g_build_filename(szHomeDirectory, name, NULL);
    exists = g_file_test(filename, G_FILE_TEST_EXISTS);
    buf = g_strdup_printf("PySQLiteConnect(r'%s')", filename);

    /* Connect to database */
    con = PyRun_String(buf, Py_eval_input, pdict, pdict);
    g_free(name);
    g_free(filename);
    g_free(buf);
    if (con == NULL) {
        PyErr_Print();
        return -1;
    } else if (con == Py_None) {
        outputl(_("Error connecting to database"));
        return -1;
    }
    if (!exists) {              /* Empty database file created - create tables */
        return 0;
    } else
        return 1;
}
#endif

static void
PyDisconnect(void)
{
    if (!PyRun_String("PyDisconnect()", Py_eval_input, pdict, pdict))
        PyErr_Print();
}

RowSet *
PySelect(const char *str)
{
    PyObject *rs;
    char *buf = g_strdup_printf("PySelect(\"%s\")", str);
    /* Remove any new lines from query string */
    char *ppch = buf;
    while (*ppch) {
        if (strchr("\t\n\r\v\f", *ppch))
            *ppch = ' ';
        ppch++;
    }

    /* Run select */
    rs = PyRun_String(buf, Py_eval_input, pdict, pdict);
    g_free(buf);

    if (rs) {
        return ConvertPythonToRowset(rs);
    } else {
        PyErr_Print();
        return NULL;
    }
}

int
PyUpdateCommand(const char *str)
{
    char *buf = g_strdup_printf("PyUpdateCommand(\"%s\")", str);
    /* Run update */
    PyObject *ret = PyRun_String(buf, Py_eval_input, pdict, pdict);
    g_free(buf);
    if (!ret) {
        PyErr_Print();
        return FALSE;
    } else
        return TRUE;
}

static void
PyCommit(void)
{
    if (!PyRun_String("PyCommit()", Py_eval_input, pdict, pdict))
        PyErr_Print();
}

RowSet *
ConvertPythonToRowset(PyObject * v)
{
    RowSet *pRow;
    Py_ssize_t row, col;
    int i, j;
    if (PyInt_Check(v)) {
        if (PyInt_AsLong(v) != 0)
            outputerrf(_("unexpected rowset error"));
        return NULL;
    }

    if (!PySequence_Check(v)) {
        outputerrf(_("invalid Python return"));
        return NULL;
    }

    row = PySequence_Size(v);
    col = 0;
    if (row > 0) {
        PyObject *cols = PySequence_GetItem(v, 0);
        if (!PySequence_Check(cols)) {
            outputerrf(_("invalid Python return"));
            return NULL;
        } else
            col = PySequence_Size(cols);
    }

    pRow = MallocRowset((size_t) row, (size_t) col);
    for (i = 0; i < (int) pRow->rows; i++) {
        PyObject *e = PySequence_GetItem(v, i);

        if (!e) {
            outputf(_("Error getting item %d\n"), i);
            continue;
        }

        if (PySequence_Check(e)) {
            for (j = 0; j < (int) pRow->cols; j++) {
                char buf[1024];
                PyObject *e2 = PySequence_GetItem(e, j);

                if (!e2) {
                    outputf(_("Error getting sub item (%d, %d)\n"), i, j);
                    continue;
                }
                if (PyUnicode_Check(e2))
                    strcpy(buf, PyBytes_AsString(PyUnicode_AsUTF8String(e2)));
                else if (PyBytes_Check(e2))
                    strcpy(buf, PyBytes_AsString(e2));
                else if (PyInt_Check(e2) || PyLong_Check(e2)
                         || !StrCaseCmp(e2->ob_type->tp_name, "Decimal"))       /* Not sure how to check for decimal type directly */
                    sprintf(buf, "%ld", PyInt_AsLong(e2));
                else if (PyFloat_Check(e2))
                    sprintf(buf, "%.4f", PyFloat_AsDouble(e2));
                else if (e2 == Py_None)
                    sprintf(buf, "[%s]", _("none"));
                else
                    sprintf(buf, "[%s]", _("unknown type"));

                SetRowsetData(pRow, (size_t) i, (size_t) j, buf);

                Py_DECREF(e2);
            }
        } else {
            outputf(_("Item %d is not a list\n"), i);
        }

        Py_DECREF(e);
    }
    return pRow;
}

GList *
PyMySQLGetDatabaseList(const char *user, const char *password, const char *hostname)
{
    PyObject *rs;

    if (PyMySQLConnect("", user, password, hostname) < 0)
        return NULL;

    rs = PyRun_String("PyUpdateCommandReturn(\"Show databases\")", Py_eval_input, pdict, pdict);
    if (rs) {
        unsigned int i;
        GList *glist = NULL;
        RowSet *list = ConvertPythonToRowset(rs);
        if (!list)
            return NULL;
        for (i = 0; i < list->rows; i++)
            glist = g_list_append(glist, g_strdup(list->data[i][0]));
        FreeRowset(list);
        return glist;
    } else {
        PyErr_Print();
        return NULL;
    }
}

#if !defined(WIN32)
GList *
PyPostgreGetDatabaseList(const char *user, const char *password, const char *hostname)
{
    RowSet *rs;

    if (PyPostgreConnect("", user, password, hostname) < 0)
        return NULL;

    rs = PySelect("datname from pg_database");
    if (rs) {
        unsigned int i;
        GList *glist = NULL;
        for (i = 1; i < rs->rows; i++)
            glist = g_list_append(glist, g_strdup(rs->data[i][0]));
        FreeRowset(rs);
        return glist;
    } else {
        PyErr_Print();
        return NULL;
    }
}
#endif

int
PyMySQLDeleteDatabase(const char *dbfilename, const char *user, const char *password, const char *hostname)
{
    char *buf;
    int ret;
    if (PyMySQLConnect(dbfilename, user, password, hostname) < 0)
        return FALSE;

    buf = g_strdup_printf("DROP DATABASE %s", dbfilename);
    ret = PyUpdateCommand(buf);
    g_free(buf);
    return ret;
}

#if !defined(WIN32)
int
PyPostgreDeleteDatabase(const char *dbfilename, const char *user, const char *password, const char *hostname)
{
    char *buf;
    int ret;
    if (PyPostgreConnect("postgres", user, password, hostname) < 0)
        return FALSE;

    PyUpdateCommand("END");
    buf = g_strdup_printf("DROP DATABASE %s", dbfilename);
    ret = PyUpdateCommand(buf);
    g_free(buf);
    return ret;
}
#endif

#endif

#if USE_SQLITE

#include <sqlite3.h>

static sqlite3 *connection;

int
SQLiteConnect(const char *dbfilename, const char *UNUSED(user), const char *UNUSED(password),
              const char *UNUSED(hostname))
{
    char *name, *filename;
    int exists, ret;

    name = g_strdup_printf("%s.db", dbfilename);
    filename = g_build_filename(szHomeDirectory, name, NULL);
    exists = g_file_test(filename, G_FILE_TEST_EXISTS);

    ret = sqlite3_open(filename, &connection);

    g_free(name);
    g_free(filename);

    if (ret == SQLITE_OK)
        return exists ? 1 : 0;
    else
        return -1;
}

static void
SQLiteDisconnect(void)
{
    if (sqlite3_close(connection) != SQLITE_OK)
        outputerrf("SQL error: %s in sqlite3_close()", sqlite3_errmsg(connection));
}

RowSet *
SQLiteSelect(const char *str)
{
    size_t i, row;
    int ret;
    char *buf = g_strdup_printf("Select %s;", str);
    RowSet *rs = NULL;

    sqlite3_stmt *pStmt;
    ret = sqlite3_prepare(connection, buf, -1, &pStmt, NULL);
    g_free(buf);
    if (ret == SQLITE_OK) {
        size_t numCols = (size_t) sqlite3_column_count(pStmt);
        size_t numRows = 0;
        while ((ret = sqlite3_step(pStmt)) == SQLITE_ROW)
            numRows++;
        if (sqlite3_reset(pStmt) != SQLITE_OK)
            outputerrf("SQL error: %s in sqlite3_reset()", sqlite3_errmsg(connection));
        rs = MallocRowset(numRows + 1, numCols);        /* first row is headings */

        for (i = 0; i < numCols; i++)
            SetRowsetData(rs, 0, i, sqlite3_column_name(pStmt, (int) i));

        row = 0;

        while ((ret = sqlite3_step(pStmt)) == SQLITE_ROW) {
            row++;
            for (i = 0; i < numCols; i++)
                SetRowsetData(rs, row, i, (const char *) sqlite3_column_text(pStmt, (int) i));
        }
        if (ret == SQLITE_DONE)
            ret = SQLITE_OK;
    }
    if (ret != SQLITE_OK)
        outputerrf("SQL error: %s\nfrom '%s'", sqlite3_errmsg(connection), str);

    if (sqlite3_finalize(pStmt) != SQLITE_OK)
        outputerrf("SQL error: %s in sqlite3_finalize()", sqlite3_errmsg(connection));
    return rs;
}

int
SQLiteUpdateCommand(const char *str)
{
    char *zErrMsg;
    int ret = sqlite3_exec(connection, str, NULL, NULL, &zErrMsg);
    if (ret != SQLITE_OK) {
        outputerrf("SQL error: %s\nfrom '%s'", zErrMsg, str);
        sqlite3_free(zErrMsg);
    }
    return (ret == SQLITE_OK);
}

static void
SQLiteCommit(void)
{                               /* No transaction in sqlite by default */
}
#endif

#if NUM_PROVIDERS
GList *
SQLiteGetDatabaseList(const char *UNUSED(user), const char *UNUSED(password), const char *UNUSED(hostname))
{
    GList *glist = NULL;
    GDir *dir = g_dir_open(szHomeDirectory, 0, NULL);
    if (dir) {
        const char *filename;
        while ((filename = g_dir_read_name(dir)) != NULL) {
            size_t len = strlen(filename);
            if (len > 3 && !StrCaseCmp(filename + len - 3, ".db")) {
                char *db = g_strdup(filename);
                db[len - 3] = '\0';
                glist = g_list_append(glist, db);
            }
        }
        g_dir_close(dir);
    }
    return glist;
}

int
SQLiteDeleteDatabase(const char *dbfilename, const char *UNUSED(user), const char *UNUSED(password),
                     const char *UNUSED(hostname))
{
    char *name, *filename;
    int ret;
    name = g_strdup_printf("%s.db", dbfilename);
    filename = g_build_filename(szHomeDirectory, name, NULL);
    /* Delete database file */
    ret = g_unlink(filename);

    g_free(name), g_free(filename);
    return (ret == 0);
}
#endif
