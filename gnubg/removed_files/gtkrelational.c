/*
 * gtkrelational.c
 *
 * by Christian Anthon <anthon@kiku.dk>, 2006.
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
 * $Id: gtkrelational.c,v 1.42 2014/07/20 21:04:53 plm Exp $
 */

#include "config.h"
#include "backgammon.h"
#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "gtkrelational.h"
#include "gtkwindows.h"
#include "gtklocdefs.h"

enum {
    COLUMN_NICK,
    COLUMN_GNUE,
    COLUMN_GCHE,
    COLUMN_GCUE,
    COLUMN_SNWE,
    COLUMN_SCHE,
    COLUMN_SCUE,
    COLUMN_WRPA,
    COLUMN_WRTA,
    COLUMN_WDTG,
    COLUMN_WDBD,
    COLUMN_MDAC,
    COLUMN_MDBC,
    COLUMN_LUCK,
    NUM_COLUMNS
};

static gchar *titles[] = {
    N_("Nick"),
    N_("Gnu\nErr"),
    N_("Gnu\nMove"),
    N_("Gnu\nCube"),
    N_("Snw\nErr"),
    N_("Snw\nMove"),
    N_("Snw\nCube"),
    N_("Pass"),
    N_("Take"),
    N_("WDb\nPass"),
    N_("WDb\nTake"),
    N_("MDb\nPass"),
    N_("MDb\nTake"),
    N_("Luck")
};

static GtkWidget *pwPlayerName;
static GtkWidget *pwPlayerNotes;
static GtkWidget *pwQueryText;
static GtkWidget *pwQueryResult = NULL;
static GtkWidget *pwQueryBox;

static GtkListStore *playerStore;
static GtkListStore *dbStore;
static GtkTreeIter selected_iter;
static int optionsValid;
static GtkWidget *playerTreeview, *adddb, *deldb, *gameStats, *dbList, *dbtype, *user, *password, *hostname, *login, *helptext;

void CheckDatabase(const char *database);
static void DBListSelected(GtkTreeView * treeview, gpointer userdata);

#define PACK_OFFSET 4
#define OUTSIDE_FRAME_GAP PACK_OFFSET
#define INSIDE_FRAME_GAP PACK_OFFSET
#define NAME_NOTES_VGAP PACK_OFFSET
#define BUTTON_GAP PACK_OFFSET
#define QUERY_BORDER 1

static GtkTreeModel *
create_model(void)
{
    GtkTreeIter iter;
    RowSet *rs;

    int moves[4];
    unsigned int i, j;
    gfloat stats[14];

    /* create list store */
    playerStore = gtk_list_store_new(NUM_COLUMNS,
                                     G_TYPE_STRING,
                                     G_TYPE_FLOAT,
                                     G_TYPE_FLOAT,
                                     G_TYPE_FLOAT,
                                     G_TYPE_FLOAT,
                                     G_TYPE_FLOAT,
                                     G_TYPE_FLOAT,
                                     G_TYPE_FLOAT,
                                     G_TYPE_FLOAT,
                                     G_TYPE_FLOAT, G_TYPE_FLOAT, G_TYPE_FLOAT, G_TYPE_FLOAT, G_TYPE_FLOAT);

    /* prepare the sql query */
    rs = RunQuery("name,"
                  "SUM(total_moves),"
                  "SUM(unforced_moves),"
                  "SUM(close_cube_decisions),"
                  "SUM(snowie_moves),"
                  "SUM(error_missed_doubles_below_cp_normalised),"
                  "SUM(error_missed_doubles_above_cp_normalised),"
                  "SUM(error_wrong_doubles_below_dp_normalised),"
                  "SUM(error_wrong_doubles_above_tg_normalised),"
                  "SUM(error_wrong_takes_normalised),"
                  "SUM(error_wrong_passes_normalised),"
                  "SUM(cube_error_total_normalised),"
                  "SUM(chequer_error_total_normalised),"
                  "SUM(luck_total_normalised) " "FROM matchstat NATURAL JOIN player group by name");
    if (!rs)
        return 0;

    if (rs->rows < 2) {
        GTKMessage(_("No data in database"), DT_INFO);
        return 0;
    }

    for (j = 1; j < rs->rows; ++j) {
        for (i = 1; i < 5; ++i)
            moves[i - 1] = (int) strtol(rs->data[j][i], NULL, 0);

        for (i = 5; i < 14; ++i)
            stats[i - 5] = (float) g_strtod(rs->data[j][i], NULL);

        gtk_list_store_append(playerStore, &iter);
        gtk_list_store_set(playerStore, &iter,
                           COLUMN_NICK,
                           rs->data[j][0],
                           COLUMN_GNUE,
                           Ratio(stats[6] + stats[7], moves[1] + moves[2]) * 1000.0f,
                           COLUMN_GCHE,
                           Ratio(stats[7], moves[1]) * 1000.0f,
                           COLUMN_GCUE,
                           Ratio(stats[6], moves[2]) * 1000.0f,
                           COLUMN_SNWE,
                           Ratio(stats[6] + stats[7], moves[3]) * 1000.0f,
                           COLUMN_SCHE,
                           Ratio(stats[7], moves[3]) * 1000.0f,
                           COLUMN_SCUE,
                           Ratio(stats[6], moves[3]) * 1000.0f,
                           COLUMN_WRPA,
                           Ratio(stats[5], moves[3]) * 1000.0f,
                           COLUMN_WRTA,
                           Ratio(stats[4], moves[3]) * 1000.0f,
                           COLUMN_WDTG,
                           Ratio(stats[3], moves[3]) * 1000.0f,
                           COLUMN_WDBD,
                           Ratio(stats[2], moves[3]) * 1000.0f,
                           COLUMN_MDAC,
                           Ratio(stats[1], moves[3]) * 1000.0f,
                           COLUMN_MDBC,
                           Ratio(stats[0], moves[3]) * 1000.0f, COLUMN_LUCK, Ratio(stats[8], moves[0]) * 1000.0f, -1);
    }
    FreeRowset(rs);
    return GTK_TREE_MODEL(playerStore);
}

static void
cell_data_func(GtkTreeViewColumn * UNUSED(col),
               GtkCellRenderer * renderer, GtkTreeModel * model, GtkTreeIter * iter, gpointer column)
{
    gfloat data;
    gchar buf[20];

    /* get data pointed to by column */
    gtk_tree_model_get(model, iter, GPOINTER_TO_INT(column), &data, -1);

    /* format the data to two digits */
    g_snprintf(buf, sizeof(buf), "%.2f", data);

    /* render the string right aligned */
    g_object_set(renderer, "text", buf, NULL);
    g_object_set(renderer, "xalign", 1.0, NULL);
}

static void
add_columns(GtkTreeView * treeview)
{
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    gint i;

    renderer = gtk_cell_renderer_text_new();

    column = gtk_tree_view_column_new_with_attributes("Nick", renderer, "text", COLUMN_NICK, NULL);
    gtk_tree_view_column_set_sort_column_id(column, 0);
    gtk_tree_view_append_column(treeview, column);

    for (i = 1; i < NUM_COLUMNS; i++) {
        column = gtk_tree_view_column_new();
        renderer = gtk_cell_renderer_text_new();
        gtk_tree_view_column_pack_end(column, renderer, TRUE);
        gtk_tree_view_column_set_cell_data_func(column, renderer, cell_data_func, GINT_TO_POINTER(i), NULL);
        gtk_tree_view_column_set_sort_column_id(column, i);
        gtk_tree_view_column_set_title(column, gettext(titles[i]));
        gtk_tree_view_append_column(treeview, column);
    }
}

static GtkWidget *
do_list_store(void)
{
    GtkTreeModel *model;
    GtkWidget *treeview;

    model = create_model();
    if (!model)
        return NULL;

    treeview = gtk_tree_view_new_with_model(model);
    g_object_unref(model);
    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(treeview), TRUE);
    gtk_tree_view_set_search_column(GTK_TREE_VIEW(treeview), COLUMN_NICK);


    add_columns(GTK_TREE_VIEW(treeview));

    return treeview;
}

static char *
GetSelectedPlayer(void)
{
    char *name;
    GtkTreeModel *model;
    GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(playerTreeview));
    if (gtk_tree_selection_count_selected_rows(sel) != 1)
        return NULL;

    model = gtk_tree_view_get_model(GTK_TREE_VIEW(playerTreeview));
    gtk_tree_selection_get_selected(sel, &model, &selected_iter);

    gtk_tree_model_get(model, &selected_iter, COLUMN_NICK, &name, -1);
    return name;
}

static void
ShowRelationalSelect(GtkWidget * UNUSED(pw), int UNUSED(y), int UNUSED(x),
                     GdkEventButton * UNUSED(peb), GtkWidget * UNUSED(pwCopy))
{
    char *pName = GetSelectedPlayer();
    RowSet *rs;
    char *query;

    gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(pwPlayerNotes)), "", -1);

    if (!pName)
        return;

    query = g_strdup_printf("player_id, name, notes FROM player WHERE player.name = '%s'", pName);
    g_free(pName);

    rs = RunQuery(query);
    g_free(query);
    if (!rs) {
        gtk_entry_set_text(GTK_ENTRY(pwPlayerName), "");
        return;
    }

    g_assert(rs->rows == 2);    /* Should be exactly one entry */

    gtk_entry_set_text(GTK_ENTRY(pwPlayerName), rs->data[1][1]);
    gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(pwPlayerNotes)), rs->data[1][2], -1);

    FreeRowset(rs);
}

static void
ShowRelationalClicked(GtkTreeView * UNUSED(treeview), GtkTreePath * UNUSED(path),
                      GtkTreeViewColumn * UNUSED(col), gpointer UNUSED(userdata))
{
    gchar *name = GetSelectedPlayer();
    if (!name)
        return;

    CommandRelationalShowDetails(name);
    g_free(name);
}

static GtkWidget *
GtkRelationalShowStats(void)
{
    GtkWidget *scrolledWindow;

    scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolledWindow), GTK_SHADOW_IN);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindow), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolledWindow), GTK_SHADOW_IN);

    playerTreeview = do_list_store();
    g_signal_connect(playerTreeview, "row-activated", (GCallback) ShowRelationalClicked, NULL);
    gtk_container_add(GTK_CONTAINER(scrolledWindow), playerTreeview);
    g_signal_connect(playerTreeview, "cursor-changed", G_CALLBACK(ShowRelationalSelect), NULL);

    return scrolledWindow;
}

extern void
GtkRelationalAddMatch(gpointer UNUSED(p), guint UNUSED(n), GtkWidget * UNUSED(pw))
{
    CommandRelationalAddMatch(NULL);
    outputx();
}

static GtkWidget *
GetRelList(RowSet * pRow)
{
    unsigned int i, j;
    GtkListStore *store;
    GType *types;
    GtkTreeIter iter;
    GtkCellRenderer *renderer;
    GtkWidget *treeview;

    unsigned int cols = pRow ? (unsigned int) pRow->cols : 0;
    unsigned int rows = pRow ? (unsigned int) pRow->rows : 0;

    if (!pRow || !rows || !cols)
        return gtk_label_new(_("Search failed or empty."));

    types = g_new(GType, cols);
    for (j = 0; j < cols; j++)
        types[j] = G_TYPE_STRING;
    store = gtk_list_store_newv(cols, types);
    g_free(types);

    for (i = 1; i < rows; i++) {
        gtk_list_store_append(store, &iter);
        for (j = 0; j < cols; j++)
            gtk_list_store_set(store, &iter, j, pRow->data[i][j], -1);
    }
    treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    g_object_unref(store);
    renderer = gtk_cell_renderer_text_new();
    for (j = 0; j < cols; j++)
        gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview), -1, pRow->data[0][j], renderer, "text", j,
                                                    NULL);
    return treeview;
}

static void
ShowRelationalErase(GtkWidget * UNUSED(pw), GtkWidget * UNUSED(notused))
{
    char *buf;
    gchar *player = GetSelectedPlayer();
    if (!player)
        return;

    buf = g_strdup_printf(_("Remove all data for %s?"), player);
    if (!GetInputYN(buf))
        return;

    sprintf(buf, "\"%s\"", player);
    CommandRelationalErase(buf);
    g_free(buf);

    gtk_list_store_remove(GTK_LIST_STORE(playerStore), &selected_iter);
}

static char *
GetText(GtkTextView * pwText)
{
    GtkTextIter start, end;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(pwText);
    char *pch;

    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_get_end_iter(buffer, &end);
    pch = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
    return pch;
}

static void
UpdatePlayerDetails(GtkWidget * UNUSED(pw), GtkWidget * UNUSED(notused))
{
    char *notes;
    const char *newname;
    gchar *oldname = GetSelectedPlayer();
    if (!oldname)
        return;

    notes = GetText(GTK_TEXT_VIEW(pwPlayerNotes));
    newname = gtk_entry_get_text(GTK_ENTRY(pwPlayerName));
    if (RelationalUpdatePlayerDetails(oldname, newname, notes) != 0)
        gtk_list_store_set(GTK_LIST_STORE(playerStore), &selected_iter, 0, newname, -1);

    g_free(notes);
    g_free(oldname);
}

static void
RelationalQuery(GtkWidget * UNUSED(pw), GtkWidget * UNUSED(pwVbox))
{
    RowSet *rs;
    char *pch, *query;

    pch = GetText(GTK_TEXT_VIEW(pwQueryText));

    if (!StrNCaseCmp("select ", pch, strlen("select ")))
        query = pch + strlen("select ");
    else
        query = pch;

    rs = RunQuery(query);
    if (pwQueryResult)
        gtk_widget_destroy(pwQueryResult);
    pwQueryResult = GetRelList(rs);
    gtk_box_pack_start(GTK_BOX(pwQueryBox), pwQueryResult, TRUE, TRUE, 0);
    gtk_widget_show(pwQueryResult);
    if (rs)
        FreeRowset(rs);

    g_free(pch);
}

static DBProvider *
GetSelectedDBType(void)
{
    DBProviderType dbType = (DBProviderType) gtk_combo_box_get_active(GTK_COMBO_BOX(dbtype));
    return GetDBProvider(dbType);
}

static void
TryConnection(DBProvider * pdb, GtkWidget * dbList)
{
    const char *msg;
    DBProviderType dbType = (DBProviderType) gtk_combo_box_get_active(GTK_COMBO_BOX(dbtype));

    gtk_list_store_clear(GTK_LIST_STORE(dbStore));
    msg = TestDB(dbType);
    gtk_widget_set_sensitive(login, FALSE);
    if (msg) {
        gtk_label_set_text(GTK_LABEL(helptext), msg);
        optionsValid = FALSE;
        gtk_widget_set_sensitive(adddb, FALSE);
        gtk_widget_set_sensitive(deldb, FALSE);
    } else {                    /* Test ok */
        GList *pl = pdb->GetDatabaseList(pdb->username, pdb->password, pdb->hostname);
        if (g_list_find_custom(pl, pdb->database, (GCompareFunc) g_ascii_strncasecmp) == NULL) {        /* Somehow selected database not in list, so add it */
            pl = g_list_append(pl, g_strdup(pdb->database));
        }
        while (pl) {
            int ok, seldb;
            GtkTreeIter iter;
            char *database = (char *) pl->data;
            seldb = !StrCaseCmp(database, pdb->database);
            if (seldb)
                ok = TRUE;
            else {
                const char *tmpDatabase = pdb->database;
                pdb->database = database;
                ok = (TestDB(dbType) == NULL);
                pdb->database = tmpDatabase;
            }
            if (ok) {
                gtk_list_store_append(GTK_LIST_STORE(dbStore), &iter);
                gtk_list_store_set(GTK_LIST_STORE(dbStore), &iter, 0, database, -1);
                if (seldb) {
                    gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(dbList)), &iter);
                    gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(dbList),
                                                 gtk_tree_model_get_path(gtk_tree_view_get_model(GTK_TREE_VIEW(dbList)),
                                                                         &iter), NULL, TRUE, 1, 0);
                }
            }
            g_free(database);
            pl = pl->next;
        }
        g_list_free(pl);
        CheckDatabase(pdb->database);
    }
}

static void
CredentialsChanged(void)
{
    gtk_widget_set_sensitive(login, TRUE);
}

static void
LoginClicked(GtkButton * UNUSED(button), gpointer dbList)
{
    const char *tmpUser, *tmpPass, *tmpHost;
    DBProvider *pdb = GetSelectedDBType();

    if (pdb == NULL)
        return;

    tmpUser = pdb->username, tmpPass = pdb->password, tmpHost = pdb->hostname;

    pdb->username = gtk_entry_get_text(GTK_ENTRY(user));
    pdb->password = gtk_entry_get_text(GTK_ENTRY(password));
    pdb->hostname = gtk_entry_get_text(GTK_ENTRY(hostname));

    TryConnection(pdb, dbList);

    pdb->username = tmpUser, pdb->password = tmpPass, pdb->hostname = tmpHost;
}

static void
TypeChanged(GtkComboBox * UNUSED(widget), gpointer dbList)
{
    DBProvider *pdb = GetSelectedDBType();

    if (pdb == NULL)
        return;

    if (pdb->HasUserDetails) {
        gtk_widget_set_sensitive(user, TRUE);
        gtk_widget_set_sensitive(password, TRUE);
        gtk_widget_set_sensitive(hostname, TRUE);
        gtk_entry_set_text(GTK_ENTRY(user), pdb->username);
        gtk_entry_set_text(GTK_ENTRY(password), pdb->password);
        if (pdb->hostname)
            gtk_entry_set_text(GTK_ENTRY(hostname), pdb->hostname);
        else
            gtk_entry_set_text(GTK_ENTRY(hostname), "");
    } else {
        gtk_widget_set_sensitive(user, FALSE);
        gtk_widget_set_sensitive(password, FALSE);
        gtk_widget_set_sensitive(hostname, FALSE);
    }

    TryConnection(pdb, dbList);
}

void
CheckDatabase(const char *database)
{
    int valid = FALSE;
    int dbok = 0;
    DBProvider *pdb = GetSelectedDBType();

    if (pdb)
        dbok =
            (pdb->Connect(database, gtk_entry_get_text(GTK_ENTRY(user)), gtk_entry_get_text(GTK_ENTRY(password)),
                          gtk_entry_get_text(GTK_ENTRY(hostname))) >= 0);

    if (!dbok)
        gtk_label_set_text(GTK_LABEL(helptext), "Failed to connect to database!");
    else {
        int version = RunQueryValue(pdb, "next_id FROM control WHERE tablename = 'version'");
        int matchcount = RunQueryValue(pdb, "count(*) FROM session");

        char *dbString, *buf, *buf2 = NULL;
        if (version < DB_VERSION)
            dbString = _("This database is from an old version of GNU Backgammon and cannot be used");
        else if (version > DB_VERSION)
            dbString = _("This database is from a new version of GNU Backgammon and cannot be used");
        else {
            if (matchcount < 0)
                dbString = _("This database structure is invalid");
            else {
                valid = TRUE;
                if (matchcount == 0)
                    dbString = _("This database contains no matches");
                else if (matchcount == 1)
                    dbString = _("This database contains 1 match");
                else {
                    buf2 = g_strdup_printf(_("This database contains %d matches\n"), matchcount);
                    dbString = buf2;
                }
            }
        }
        buf = g_strdup_printf(_("Database connection successful\n%s\n"), dbString);
        gtk_label_set_text(GTK_LABEL(helptext), buf);
        g_free(buf);
        g_free(buf2);

        pdb->Disconnect();
    }
    gtk_widget_set_sensitive(adddb, dbok);
    gtk_widget_set_sensitive(deldb, dbok);
    optionsValid = valid;
}

static char *
GetSelectedDB(GtkTreeView * treeview)
{
    GtkTreeModel *model;
    char *db = NULL;
    GtkTreeSelection *sel = gtk_tree_view_get_selection(treeview);
    if (gtk_tree_selection_count_selected_rows(sel) != 1)
        return NULL;
    gtk_tree_selection_get_selected(sel, &model, &selected_iter);
    gtk_tree_model_get(model, &selected_iter, 0, &db, -1);
    return db;
}

static void
DBListSelected(GtkTreeView * treeview, gpointer UNUSED(userdata))
{
    char *db = GetSelectedDB(treeview);
    if (db) {
        CheckDatabase(db);
        g_free(db);
    }
}

static void
AddDBClicked(GtkButton * UNUSED(button), gpointer dbList)
{
    char *dbName = GTKGetInput(_("Add Database"), _("Database Name:"), NULL);
    if (dbName) {
        DBProvider *pdb = GetSelectedDBType();
        int con = 0;
        gchar *sz;
        GtkTreeIter iter;
        GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(dbList));

        /* If the database name is already in the list, don't try to add a new one */
        if (gtk_tree_model_get_iter_first(model, &iter))
            do {
                gtk_tree_model_get(model, &iter, 0, &sz, -1);
                if (g_ascii_strcasecmp(dbName, sz) == 0) {
                    gtk_label_set_text(GTK_LABEL(helptext), _("Failed to create, database exists!"));
                    g_free(dbName);
                    return;
                }
            } while (gtk_tree_model_iter_next(model, &iter));

        if (pdb)
            con =
                pdb->Connect(dbName, gtk_entry_get_text(GTK_ENTRY(user)), gtk_entry_get_text(GTK_ENTRY(password)),
                             gtk_entry_get_text(GTK_ENTRY(hostname)));

        if (con > 0 || ((pdb) && CreateDatabase(pdb))) {
            GtkTreeIter iter;
            gtk_list_store_append(GTK_LIST_STORE(dbStore), &iter);
            gtk_list_store_set(GTK_LIST_STORE(dbStore), &iter, 0, dbName, -1);
            gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(dbList)), &iter);
            pdb->Disconnect();
            CheckDatabase(dbName);
        } else
            gtk_label_set_text(GTK_LABEL(helptext), _("Failed to create database!"));

        g_free(dbName);
    }
}

static void
DelDBClicked(GtkButton * UNUSED(button), gpointer dbList)
{
    char *db = GetSelectedDB(GTK_TREE_VIEW(dbList));
    if (db && GetInputYN(_("Are you sure you want to delete all the matches in this database?"))) {
        DBProvider *pdb = GetSelectedDBType();

        if (pdb
            && pdb->DeleteDatabase(db, gtk_entry_get_text(GTK_ENTRY(user)), gtk_entry_get_text(GTK_ENTRY(password)),
                                   gtk_entry_get_text(GTK_ENTRY(hostname)))) {
            gtk_list_store_remove(GTK_LIST_STORE(dbStore), &selected_iter);
            optionsValid = FALSE;
            gtk_widget_set_sensitive(deldb, FALSE);
            gtk_label_set_text(GTK_LABEL(helptext), _("Database successfully removed"));
            pdb->database = "gnubg";
        } else
            gtk_label_set_text(GTK_LABEL(helptext), _("Failed to delete database!"));
    }
}

extern void
RelationalOptionsShown(void)
{                               /* Setup the options when tab selected */
    gtk_combo_box_set_active(GTK_COMBO_BOX(dbtype), dbProviderType);
}

extern void
RelationalSaveOptions(void)
{
    if (optionsValid) {
        DBProviderType dbType = (DBProviderType) gtk_combo_box_get_active(GTK_COMBO_BOX(dbtype));
        SetDBSettings(dbType, GetSelectedDB(GTK_TREE_VIEW(dbList)), gtk_entry_get_text(GTK_ENTRY(user)),
                      gtk_entry_get_text(GTK_ENTRY(password)), gtk_entry_get_text(GTK_ENTRY(hostname)));

        storeGameStats = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gameStats));
    }
}

extern GtkWidget *
RelationalOptions(void)
{
    int i;
    GtkWidget *hb1, *hb2, *vb1, *vb2, *table, *lbl, *align, *help, *pwScrolled;

    dbStore = gtk_list_store_new(1, G_TYPE_STRING);
    dbList = gtk_tree_view_new_with_model(GTK_TREE_MODEL(dbStore));
    g_object_unref(dbStore);
    gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(dbList)), GTK_SELECTION_BROWSE);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(dbList), -1, _("Databases"), gtk_cell_renderer_text_new(),
                                                "text", 0, NULL);
    gtk_tree_view_set_headers_clickable(GTK_TREE_VIEW(dbList), FALSE);

    g_signal_connect(dbList, "cursor-changed", G_CALLBACK(DBListSelected), NULL);

    dbtype = gtk_combo_box_text_new();
    for (i = 0; i < NUM_PROVIDERS; i++)
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(dbtype), GetProviderName(i));
    g_signal_connect(dbtype, "changed", G_CALLBACK(TypeChanged), dbList);

    vb2 = gtk_vbox_new(FALSE, 0);
    hb2 = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vb2), hb2, FALSE, FALSE, 10);

    vb1 = gtk_vbox_new(FALSE, 0);

    hb1 = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hb1), gtk_label_new(_("DB Type")), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hb1), dbtype, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(vb1), hb1, FALSE, FALSE, 0);

    table = gtk_table_new(4, 2, FALSE);
    lbl = gtk_label_new(_("Username"));
    gtk_misc_set_alignment(GTK_MISC(lbl), 1, .5);
    gtk_table_attach(GTK_TABLE(table), lbl, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
    user = gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(user), 20);
    g_signal_connect(user, "changed", G_CALLBACK(CredentialsChanged), dbList);
    gtk_table_attach(GTK_TABLE(table), user, 1, 2, 0, 1, 0, 0, 0, 0);

    lbl = gtk_label_new(_("Password"));
    gtk_misc_set_alignment(GTK_MISC(lbl), 1, .5);
    gtk_table_attach(GTK_TABLE(table), lbl, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
    password = gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(password), 20);
    gtk_entry_set_visibility(GTK_ENTRY(password), FALSE);
    g_signal_connect(password, "changed", G_CALLBACK(CredentialsChanged), dbList);
    gtk_table_attach(GTK_TABLE(table), password, 1, 2, 1, 2, 0, 0, 0, 0);

    lbl = gtk_label_new(_("Hostname"));
    gtk_misc_set_alignment(GTK_MISC(lbl), 1, .5);
    gtk_table_attach(GTK_TABLE(table), lbl, 0, 1, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
    hostname = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(hostname), 64);
    gtk_entry_set_width_chars(GTK_ENTRY(hostname), 20);
    gtk_entry_set_visibility(GTK_ENTRY(hostname), TRUE);
    g_signal_connect(hostname, "changed", G_CALLBACK(CredentialsChanged), dbList);
    gtk_table_attach(GTK_TABLE(table), hostname, 1, 2, 2, 3, 0, 0, 0, 0);

    login = gtk_button_new_with_label("Login");
    g_signal_connect(login, "clicked", G_CALLBACK(LoginClicked), dbList);

    align = gtk_alignment_new(1, 0, 0, 0);
    gtk_container_add(GTK_CONTAINER(align), login);
    gtk_table_attach(GTK_TABLE(table), align, 1, 2, 3, 4, GTK_EXPAND | GTK_FILL, 0, 0, 0);

    gtk_box_pack_start(GTK_BOX(vb1), table, FALSE, FALSE, 4);

    gameStats = gtk_check_button_new_with_label(_("Store game stats"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gameStats), storeGameStats);
    gtk_box_pack_start(GTK_BOX(vb1), gameStats, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(hb2), vb1, FALSE, FALSE, 10);

    help = gtk_frame_new(_("Info"));
    helptext = gtk_label_new(NULL);
    gtk_misc_set_alignment(GTK_MISC(helptext), 0, 0);
    gtk_misc_set_padding(GTK_MISC(helptext), 4, 4);
    gtk_widget_set_size_request(helptext, 400, 70);
    gtk_container_add(GTK_CONTAINER(help), helptext);
    gtk_box_pack_start(GTK_BOX(vb2), help, FALSE, FALSE, 4);

    pwScrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(pwScrolled, 100, 100);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pwScrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(pwScrolled), dbList);

    vb1 = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hb2), vb1, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vb1), pwScrolled, FALSE, FALSE, 0);
    hb1 = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vb1), hb1, FALSE, FALSE, 0);
    adddb = gtk_button_new_with_label("Add database");
    g_signal_connect(adddb, "clicked", G_CALLBACK(AddDBClicked), dbList);
    gtk_box_pack_start(GTK_BOX(hb1), adddb, FALSE, FALSE, 0);
    deldb = gtk_button_new_with_label("Delete database");
    g_signal_connect(deldb, "clicked", G_CALLBACK(DelDBClicked), dbList);
    gtk_box_pack_start(GTK_BOX(hb1), deldb, FALSE, FALSE, 4);

    return vb2;
}

extern void
GtkShowRelational(gpointer UNUSED(p), guint UNUSED(n), GtkWidget * UNUSED(pw))
{
    GtkWidget *pwRun, *pwDialog, *pwHbox2, *pwVbox2,
        *pwPlayerFrame, *pwUpdate, *pwPaned, *pwVbox, *pwErase, *pwOpen, *pwn, *pwLabel, *pwScrolled, *pwHbox;
    DBProvider *pdb;
    static GtkTextBuffer *query = NULL; /*remember query */

    if (((pdb = ConnectToDB(dbProviderType)) == NULL) || RunQueryValue(pdb, "count(*) FROM player") < 2) {
        if (pdb)
            pdb->Disconnect();

        GTKMessage(_("No data in database"), DT_INFO);
        return;
    }
    pdb->Disconnect();

    pwDialog = GTKCreateDialog(_("GNU Backgammon - Database"),
                               DT_INFO, NULL, DIALOG_FLAG_MODAL | DIALOG_FLAG_MINMAXBUTTONS, NULL, NULL);

#define REL_DIALOG_HEIGHT 600
    gtk_window_set_default_size(GTK_WINDOW(pwDialog), -1, REL_DIALOG_HEIGHT);

    pwn = gtk_notebook_new();
    gtk_container_set_border_width(GTK_CONTAINER(pwn), 0);

/*******************************************************
** Start of (left hand side) of player screen...
*******************************************************/

    pwPaned = gtk_vpaned_new();
    gtk_paned_set_position(GTK_PANED(pwPaned), (int) (REL_DIALOG_HEIGHT * 0.6));
    gtk_notebook_append_page(GTK_NOTEBOOK(pwn), pwPaned, gtk_label_new(_("Players")));
    pwVbox = gtk_vbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(pwVbox), INSIDE_FRAME_GAP);
    gtk_paned_add1(GTK_PANED(pwPaned), pwVbox);
    gtk_box_pack_start(GTK_BOX(pwVbox), GtkRelationalShowStats(), TRUE, TRUE, 0);
    pwHbox2 = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(pwVbox), pwHbox2, FALSE, FALSE, 0);

    pwOpen = gtk_button_new_with_label("Open");
    g_signal_connect(G_OBJECT(pwOpen), "clicked", G_CALLBACK(ShowRelationalClicked), NULL);
    gtk_box_pack_start(GTK_BOX(pwHbox2), pwOpen, FALSE, FALSE, 0);

    pwErase = gtk_button_new_with_label("Erase");
    g_signal_connect(G_OBJECT(pwErase), "clicked", G_CALLBACK(ShowRelationalErase), NULL);
    gtk_box_pack_start(GTK_BOX(pwHbox2), pwErase, FALSE, FALSE, BUTTON_GAP);

/*******************************************************
** Start of right hand side of player screen...
*******************************************************/

    pwPlayerFrame = gtk_frame_new("Player");
    gtk_container_set_border_width(GTK_CONTAINER(pwPlayerFrame), OUTSIDE_FRAME_GAP);
    gtk_paned_add2(GTK_PANED(pwPaned), pwPlayerFrame);

    pwVbox = gtk_vbox_new(FALSE, NAME_NOTES_VGAP);
    gtk_container_set_border_width(GTK_CONTAINER(pwVbox), INSIDE_FRAME_GAP);
    gtk_container_add(GTK_CONTAINER(pwPlayerFrame), pwVbox);

    pwHbox2 = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(pwVbox), pwHbox2, FALSE, FALSE, 0);

    pwLabel = gtk_label_new("Name");
    gtk_box_pack_start(GTK_BOX(pwHbox2), pwLabel, FALSE, FALSE, 0);
    pwPlayerName = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(pwHbox2), pwPlayerName, TRUE, TRUE, 0);

    pwVbox2 = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(pwVbox), pwVbox2, TRUE, TRUE, 0);

    pwLabel = gtk_label_new("Notes");
    gtk_misc_set_alignment(GTK_MISC(pwLabel), 0, 0.5);
    gtk_box_pack_start(GTK_BOX(pwVbox2), pwLabel, FALSE, FALSE, 0);

    pwPlayerNotes = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(pwPlayerNotes), TRUE);

    pwScrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pwScrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(pwScrolled), pwPlayerNotes);
    gtk_box_pack_start(GTK_BOX(pwVbox2), pwScrolled, TRUE, TRUE, 0);

    pwHbox2 = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(pwVbox), pwHbox2, FALSE, FALSE, 0);

    pwUpdate = gtk_button_new_with_label("Update Details");
    gtk_box_pack_start(GTK_BOX(pwHbox2), pwUpdate, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(pwUpdate), "clicked", G_CALLBACK(UpdatePlayerDetails), NULL);

/*******************************************************
** End of right hand side of player screen...
*******************************************************/

    /* Query sheet */
    pwVbox = gtk_vbox_new(FALSE, 0);
    gtk_notebook_append_page(GTK_NOTEBOOK(pwn), pwVbox, gtk_label_new(_("Query")));
    gtk_container_set_border_width(GTK_CONTAINER(pwVbox), INSIDE_FRAME_GAP);

    pwLabel = gtk_label_new("Query text");
    gtk_misc_set_alignment(GTK_MISC(pwLabel), 0, 0.5);
    gtk_box_pack_start(GTK_BOX(pwVbox), pwLabel, FALSE, FALSE, 0);

    if (!query) {
        query = gtk_text_buffer_new(NULL);
        pwQueryText = gtk_text_view_new_with_buffer(query);
        gtk_text_buffer_set_text(query,
                                 "s.session_id, s.length, p1.name, p2.name from player p1, player p2 join session s on s.player_id0 = p1.player_id and s.player_id1 = p2.player_id",
                                 -1);
    } else
        pwQueryText = gtk_text_view_new_with_buffer(query);


    gtk_text_view_set_border_window_size(GTK_TEXT_VIEW(pwQueryText), GTK_TEXT_WINDOW_TOP, QUERY_BORDER);
    gtk_text_view_set_border_window_size(GTK_TEXT_VIEW(pwQueryText), GTK_TEXT_WINDOW_RIGHT, QUERY_BORDER);
    gtk_text_view_set_border_window_size(GTK_TEXT_VIEW(pwQueryText), GTK_TEXT_WINDOW_BOTTOM, QUERY_BORDER);
    gtk_text_view_set_border_window_size(GTK_TEXT_VIEW(pwQueryText), GTK_TEXT_WINDOW_LEFT, QUERY_BORDER);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(pwQueryText), TRUE);
    gtk_box_pack_start(GTK_BOX(pwVbox), pwQueryText, FALSE, FALSE, 0);
    gtk_widget_set_size_request(pwQueryText, 250, 80);

    pwHbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(pwVbox), pwHbox, FALSE, FALSE, 0);
    pwLabel = gtk_label_new("Result");
    gtk_misc_set_alignment(GTK_MISC(pwLabel), 0, 0.5);
    gtk_box_pack_start(GTK_BOX(pwHbox), pwLabel, TRUE, TRUE, 0);

    pwRun = gtk_button_new_with_label("Run Query");
    g_signal_connect(G_OBJECT(pwRun), "clicked", G_CALLBACK(RelationalQuery), pwVbox);
    gtk_box_pack_start(GTK_BOX(pwHbox), pwRun, FALSE, FALSE, 0);

    pwQueryBox = gtk_vbox_new(FALSE, 0);
    pwScrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pwScrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(pwScrolled), pwQueryBox);
    gtk_box_pack_start(GTK_BOX(pwVbox), pwScrolled, TRUE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(DialogArea(pwDialog, DA_MAIN)), pwn);

    GTKRunDialog(pwDialog);
}

extern void
GtkShowQuery(RowSet * pRow)
{
    GtkWidget *pwDialog;

    pwDialog = GTKCreateDialog(_("GNU Backgammon - Database Result"), DT_INFO, NULL, DIALOG_FLAG_MODAL, NULL, NULL);

    gtk_container_add(GTK_CONTAINER(DialogArea(pwDialog, DA_MAIN)), GetRelList(pRow));

    GTKRunDialog(pwDialog);
}
