/*
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
 * $Id: dbprovider.h,v 1.17 2013/06/16 02:16:10 mdpetch Exp $
 */

#ifndef DBPROVIDER_H
#define DBPROVIDER_H

#include <stdio.h>
#include <glib.h>
extern int storeGameStats;

typedef struct _RowSet {
    size_t cols, rows;
    char ***data;
    size_t *widths;
} RowSet;

typedef struct _DBProvider {
    int (*Connect) (const char *database, const char *user, const char *password, const char *hostname);
    void (*Disconnect) (void);
    RowSet *(*Select) (const char *str);
    int (*UpdateCommand) (const char *str);
    void (*Commit) (void);
    GList *(*GetDatabaseList) (const char *user, const char *password, const char *hostname);
    int (*DeleteDatabase) (const char *database, const char *user, const char *password, const char *hostname);

    const char *name;
    const char *shortname;
    const char *desc;
    int HasUserDetails;
    int storeGameStats;
    const char *database;
    const char *username;
    const char *password;
    const char *hostname;
} DBProvider;

typedef enum _DBProviderType {
    INVALID_PROVIDER = -1,
#if USE_SQLITE
    SQLITE,
#endif
#if USE_PYTHON
#if !USE_SQLITE
    PYTHON_SQLITE,
#endif
    PYTHON_MYSQL,
#if !defined(WIN32)
    PYTHON_POSTGRES
#endif
#endif
} DBProviderType;

#if USE_PYTHON
#if !defined(WIN32)
#define NUM_PROVIDERS 3
#else
#define NUM_PROVIDERS 2
#endif
#elif USE_SQLITE
#define NUM_PROVIDERS 1
#else
#define NUM_PROVIDERS 0
#endif

extern DBProviderType dbProviderType;
extern DBProvider *GetDBProvider(DBProviderType dbType);
extern const char *TestDB(DBProviderType dbType);
DBProvider *ConnectToDB(DBProviderType dbType);
void SetDBType(const char *type);
void SetDBSettings(DBProviderType dbType, const char *database, const char *user, const char *password,
                   const char *hostname);
void RelationalSaveSettings(FILE * pf);
void SetDBParam(const char *db, const char *key, const char *value);
extern int CreateDatabase(DBProvider * pdb);
const char *GetProviderName(int i);
extern RowSet *RunQuery(const char *sz);
extern int RunQueryValue(const DBProvider * pdb, const char *query);
extern void FreeRowset(RowSet * pRow);
#endif
