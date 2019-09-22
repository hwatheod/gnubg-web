/*lint -e818 */
/*
 * gtkmovelistctrl.c
 * by Jon Kinsey, 2005
 *
 * Analysis move list control
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
 * $Id: gtkmovelistctrl.c,v 1.25 2014/07/20 21:03:22 plm Exp $
 */

#include "config.h"
#include "gtklocdefs.h"

#include "gtkgame.h"
#include "gtkmovelistctrl.h"
#include "format.h"
#include "drawboard.h"

GdkColor wlCol;

static void custom_cell_renderer_movelist_init(CustomCellRendererMovelist * cellprogress);
static void custom_cell_renderer_movelist_class_init(CustomCellRendererMovelistClass * klass);
static void custom_cell_renderer_movelist_set_property(GObject * object,
                                                       guint param_id, const GValue * value, GParamSpec * pspec);
static void custom_cell_renderer_movelist_finalize(GObject * gobject);

/* These functions are the heart of our custom cell renderer: */
static void custom_cell_renderer_movelist_get_size(GtkCellRenderer * cell,
                                                   GtkWidget * widget,
                                                   GdkRectangle * cell_area,
                                                   gint * x_offset, gint * y_offset, gint * width, gint * height);

static void custom_cell_renderer_movelist_render(GtkCellRenderer * cell,
                                                 GdkWindow * window,
                                                 GtkWidget * widget,
                                                 GdkRectangle * background_area,
                                                 GdkRectangle * cell_area,
                                                 GdkRectangle * expose_area, GtkCellRendererState flags);
static gpointer parent_class;

/***************************************************************************
 *
 *  custom_cell_renderer_movelist_get_type: here we register our type with
 *                                          the GObject type system if we
 *                                          haven't done so yet. Everything
 *                                          else is done in the callbacks.
 *
 ***************************************************************************/

static GType
custom_cell_renderer_movelist_get_type(void)
{
    static GType cell_progress_type = 0;

    if (cell_progress_type)
        return cell_progress_type;

    {
        static const GTypeInfo cell_progress_info = {
            sizeof(CustomCellRendererMovelistClass),
            NULL,               /* base_init */
            NULL,               /* base_finalize */
            (GClassInitFunc) custom_cell_renderer_movelist_class_init,
            NULL,               /* class_finalize */
            NULL,               /* class_data */
            sizeof(CustomCellRendererMovelist),
            0,                  /* n_preallocs */
            (GInstanceInitFunc) custom_cell_renderer_movelist_init,
            NULL
        };

        /* Derive from GtkCellRenderer */
        cell_progress_type = g_type_register_static(GTK_TYPE_CELL_RENDERER,
                                                    "CustomCellRendererMovelist", &cell_progress_info, (GTypeFlags) 0);
    }

    return cell_progress_type;
}


/***************************************************************************
 *
 *  custom_cell_renderer_movelist_init: set some default properties of the
 *                                      parent (GtkCellRenderer).
 *
 ***************************************************************************/

static void
custom_cell_renderer_movelist_init(CustomCellRendererMovelist * cellrendererprogress)
{
    gtk_cell_renderer_set_padding(GTK_CELL_RENDERER(cellrendererprogress), 2, 2);
    g_object_set(GTK_CELL_RENDERER(cellrendererprogress), "mode", (int) GTK_CELL_RENDERER_MODE_INERT, NULL);
}


/***************************************************************************
 *
 *  custom_cell_renderer_movelist_class_init:
 *
 *  set up our own get_property and set_property functions, and
 *  override the parent's functions that we need to implement.
 *  And make our new "percentage" property known to the type system.
 *  If you want cells that can be activated on their own (ie. not
 *  just the whole row selected) or cells that are editable, you
 *  will need to override 'activate' and 'start_editing' as well.
 *
 ***************************************************************************/

static void
custom_cell_renderer_movelist_class_init(CustomCellRendererMovelistClass * klass)
{
    GtkCellRendererClass *cell_class = GTK_CELL_RENDERER_CLASS(klass);
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    parent_class = g_type_class_peek_parent(klass);
    object_class->finalize = custom_cell_renderer_movelist_finalize;

    /* Hook up functions to set and get our
     *   custom cell renderer properties */
    object_class->get_property = NULL;
    object_class->set_property = custom_cell_renderer_movelist_set_property;

    /* Override the two crucial functions that are the heart
     *   of a cell renderer in the parent class */
    cell_class->get_size = custom_cell_renderer_movelist_get_size;
    cell_class->render = custom_cell_renderer_movelist_render;

    /* Install our very own properties */
    g_object_class_install_property(object_class, 1,
                                    g_param_spec_pointer("movelist",
                                                         ("Move List"), ("The move list entry"), G_PARAM_WRITABLE));

    g_object_class_install_property(object_class, 2,
                                    g_param_spec_int("rank",
                                                     ("Rank"), ("The moves rank"), -1, 1000000, 0, G_PARAM_WRITABLE));
}


/***************************************************************************
 *
 *  custom_cell_renderer_movelist_finalize: free any resources here
 *
 ***************************************************************************/

static void
custom_cell_renderer_movelist_finalize(GObject * object)
{
    /*
     * CustomCellRendererMovelist *cellrendererprogress = CUSTOM_CELL_RENDERER_MOVELIST(object);
     */

    /* Free any dynamically allocated resources here */

    (*G_OBJECT_CLASS(parent_class)->finalize) (object);
}

/* Not needed
 * static void
 * custom_cell_renderer_movelist_get_property (GObject    *object,
 * guint       param_id,
 * GValue     *value,
 * GParamSpec *pspec)
 * {
 * CustomCellRendererMovelist  *cellprogress = CUSTOM_CELL_RENDERER_MOVELIST(object);
 * g_assert(param_id == 1);
 * 
 * g_value_set_pointer(value, cellprogress->pml);
 * }
 */
static void
custom_cell_renderer_movelist_set_property(GObject * object,
                                           guint param_id, const GValue * value, GParamSpec * UNUSED(notused))
{
    CustomCellRendererMovelist *cellprogress = CUSTOM_CELL_RENDERER_MOVELIST(object);
    if (param_id == 1)
        cellprogress->pml = g_value_get_pointer(value);
    else
        cellprogress->rank = (unsigned) g_value_get_int(value);
}

/***************************************************************************
 *
 *  custom_cell_renderer_movelist_new: return a new cell renderer instance
 *
 ***************************************************************************/

GtkCellRenderer *
custom_cell_renderer_movelist_new(void)
{
    return g_object_new(CUSTOM_TYPE_CELL_RENDERER_MOVELIST, NULL);
}


/***************************************************************************
 *
 *  custom_cell_renderer_movelist_get_size: calculate the size of our cell
 *                                          Padding and alignment properties
 *                                          of parent are largely ignored...
 *
 ***************************************************************************/

static int fontheight = -1, minWidth;

extern void
custom_cell_renderer_invalidate_size(void)
{
    fontheight = -1;
}

/* The layout of the control has many size variable (declard below - starting _s_)
 * The diagram here trys to help specify what each size means.  (note the _s_ has been left out)
 * B
 * A a b c c d E
 * C
 * A2 Z Z Z s Z Z Z E
 * D
 * 
 * (Y - space)
 * (m - minus)
 */
static int _s_A, _s_A2, _s_B, _s_C, _s_D, _s_E, _s_Y, _s_Z, _s_ZP, _s_a, _s_b, _s_c, _s_cP, _s_d, _s_s, _s_m;

static void
custom_cell_renderer_movelist_get_size(GtkCellRenderer * cell,
                                       GtkWidget * widget,
                                       GdkRectangle * cell_area,
                                       gint * x_offset, gint * y_offset, gint * width, gint * height)
{
    gint calc_width;
    gint calc_height;
    gfloat xalign, yalign;

    gtk_cell_renderer_get_alignment(cell, &xalign, &yalign);

    if (fontheight == -1) {     /* Calculate sizes (if not known) */
        int l1Width, l2Width;
        PangoRectangle logical_rect;
        PangoLayout *layout;
        char buf[100];

        sprintf(buf, "%.*f", fOutputDigits, 0.888888);
        layout = gtk_widget_create_pango_layout(widget, buf);
        pango_layout_get_pixel_extents(layout, NULL, &logical_rect);
        g_object_unref(layout);
        fontheight = logical_rect.height;
        _s_A = _s_E = _s_C = _s_D = fontheight / 5;
        _s_B = 0;               /* Pack lines quite closely */

        _s_Z = logical_rect.width;

        sprintf(buf, "%.*f%%", fOutputDigits > 1 ? fOutputDigits - 1 : 0, 0.888888);
        layout = gtk_widget_create_pango_layout(widget, buf);
        pango_layout_get_pixel_extents(layout, NULL, &logical_rect);
        g_object_unref(layout);
        _s_ZP = logical_rect.width;

        layout = gtk_widget_create_pango_layout(widget, " ");
        pango_layout_get_pixel_extents(layout, NULL, &logical_rect);
        g_object_unref(layout);
        _s_Y = logical_rect.width;

        layout = gtk_widget_create_pango_layout(widget, "-");
        pango_layout_get_pixel_extents(layout, NULL, &logical_rect);
        g_object_unref(layout);
        _s_m = logical_rect.width;
        _s_s = 6 * _s_Y;

        _s_A2 = _s_A + _s_Y * 4;
        l2Width = _s_A2 + 6 * _s_Z + 5 * _s_Y * 2 + _s_s;

        layout = gtk_widget_create_pango_layout(widget, "1000");
        pango_layout_get_pixel_extents(layout, NULL, &logical_rect);
        g_object_unref(layout);
        _s_a = logical_rect.width;

        sprintf(buf, "%s 3-ply", _("Cubeless"));
        layout = gtk_widget_create_pango_layout(widget, buf);
        pango_layout_get_pixel_extents(layout, NULL, &logical_rect);
        g_object_unref(layout);
        _s_b = logical_rect.width;

        sprintf(buf, "+%.*f", fOutputDigits, 0.888888);
        layout = gtk_widget_create_pango_layout(widget, buf);
        pango_layout_get_pixel_extents(layout, NULL, &logical_rect);
        g_object_unref(layout);
        _s_c = logical_rect.width;
        sprintf(buf, "0%.*f%%", fOutputDigits > 1 ? fOutputDigits - 1 : 0, 0.888888);
        layout = gtk_widget_create_pango_layout(widget, buf);
        pango_layout_get_pixel_extents(layout, NULL, &logical_rect);
        g_object_unref(layout);
        _s_cP = logical_rect.width;

        layout = gtk_widget_create_pango_layout(widget, "bar/22* 23/21* 20/18* 19/17*????");
        pango_layout_get_pixel_extents(layout, NULL, &logical_rect);
        g_object_unref(layout);
        _s_d = logical_rect.width;

        l1Width = _s_A + _s_a + _s_b + 2 * _s_c + _s_d + 4 * _s_Y;

        minWidth = (l1Width > l2Width) ? l1Width : l2Width;
    }

    calc_width = (gint) minWidth + _s_A + _s_E;
    calc_height = (gint) fontheight *2 + _s_B + _s_C + _s_D;

    if (width)
        *width = calc_width;

    if (height)
        *height = calc_height;

    if (cell_area) {
        if (x_offset) {
            *x_offset = (int) (xalign * (cell_area->width - calc_width));
            *x_offset = MAX(*x_offset, 0);
        }

        if (y_offset) {
            *y_offset = (int) (yalign * (cell_area->height - calc_height));
            *y_offset = MAX(*y_offset, 0);
        }
    }
}


/***************************************************************************
 *
 *  custom_cell_renderer_movelist_render: crucial - do the rendering.
 *
 ***************************************************************************/

static void
custom_cell_renderer_movelist_render(GtkCellRenderer * cell,
                                     GdkWindow * window,
                                     GtkWidget * widget,
                                     GdkRectangle * background_area,
                                     GdkRectangle * cell_area, GdkRectangle * expose_area, GtkCellRendererState flags)
{
    CustomCellRendererMovelist *cellprogress = CUSTOM_CELL_RENDERER_MOVELIST(cell);
    PangoLayout *layout = gtk_widget_create_pango_layout(widget, NULL);
    GdkGC *gc;
    hintdata *phd = g_object_get_data(G_OBJECT(widget), "hintdata");
    int i, x, y, selected;
    char buf[100];
    float *ar;
    GdkColor *pFontCol, *fg;
    PangoRectangle logical_rect;
    char *cmark_sz;
    char *highlight_sz;
    cubeinfo ci;
    GetMatchStateCubeInfo(&ci, &ms);
    /*lint --e(641) */
    selected = (flags & GTK_CELL_RENDERER_SELECTED) && gtk_widget_has_focus(widget);

    gc = gdk_gc_new(window);
    if (expose_area)
        gdk_gc_set_clip_rectangle(gc, expose_area);

    if (phd->piHighlight && cellprogress->rank - 1 == *phd->piHighlight)
        pFontCol = &psHighlight->fg[GTK_STATE_SELECTED];
    else
        pFontCol = NULL;

    if (!(flags & GTK_CELL_RENDERER_SELECTED)) {
        gdk_gc_set_rgb_fg_color(gc, &gtk_widget_get_style(widget)->base[GTK_STATE_NORMAL]);
        gdk_draw_rectangle(window, gc, TRUE, background_area->x, background_area->y,
                           background_area->width, background_area->height);
        gdk_gc_set_rgb_fg_color(gc, &gtk_widget_get_style(widget)->fg[GTK_STATE_NORMAL]);
    } else {                    /* Draw text in reverse colours for highlight cell */
        if (!pFontCol && selected)
            pFontCol = &gtk_widget_get_style(widget)->base[GTK_STATE_NORMAL];
    }

    /* First line of control */
    cmark_sz = cellprogress->pml->cmark ? "+" : "";
    highlight_sz = (phd->piHighlight && cellprogress->rank - 1 == *phd->piHighlight) ? "*" : "";
    if (cellprogress->rank > 0)
        sprintf(buf, "%u%s%s", cellprogress->rank, cmark_sz, highlight_sz);
    else
        sprintf(buf, "??%s%s", cmark_sz, highlight_sz);

    pango_layout_set_text(layout, buf, -1);
    pango_layout_get_pixel_extents(layout, NULL, &logical_rect);

    x = _s_A + _s_a - logical_rect.width;
    y = _s_B;

    gdk_draw_layout_with_colors(window, gc, cell_area->x + x, cell_area->y + y, layout, pFontCol, 0);

    x = _s_A + _s_a + _s_Y * 3;
    (void) FormatEval(buf, &cellprogress->pml->esMove);
    pango_layout_set_text(layout, buf, -1);
    gdk_draw_layout_with_colors(window, gc, cell_area->x + x, cell_area->y + y, layout, pFontCol, 0);

    x += _s_b + _s_Y;
    pango_layout_set_text(layout, OutputEquity(cellprogress->pml->rScore, &ci, TRUE), -1);
    gdk_draw_layout_with_colors(window, gc, cell_area->x + x, cell_area->y + y, layout, pFontCol, 0);

    if (fOutputMWC)
        x += _s_cP + _s_Y * 2;
    else
        x += _s_c + _s_Y * 2;
    if (cellprogress->rank != 1) {
        pango_layout_set_text(layout, OutputEquityDiff(cellprogress->pml->rScore, rBest, &ci), -1);
        gdk_draw_layout_with_colors(window, gc, cell_area->x + x, cell_area->y + y, layout, pFontCol, 0);
    }

    if (fOutputMWC)
        x += _s_cP + _s_Y * 2;
    else
        x += _s_c + _s_Y * 2;

    {                           /* Move text - in bold */
        PangoFontDescription *pfd = pango_context_get_font_description(pango_layout_get_context(layout));
        pango_font_description_set_weight(pfd, PANGO_WEIGHT_BOLD);
        pango_layout_set_font_description(layout, pfd);

        pango_layout_set_text(layout, FormatMove(buf, msBoard(), cellprogress->pml->anMove), -1);
        gdk_draw_layout_with_colors(window, gc, cell_area->x + x, cell_area->y + y, layout, pFontCol, 0);

        pango_font_description_set_weight(pfd, PANGO_WEIGHT_NORMAL);
        pango_layout_set_font_description(layout, pfd);
    }

    /* Second line w/l stats */

    x = _s_A2;
    y += fontheight + _s_C;

    ar = cellprogress->pml->arEvalMove;

    if (selected)
        fg = &gtk_widget_get_style(widget)->base[GTK_STATE_NORMAL];
    else
        fg = &wlCol;

    for (i = 0; i < 6; i++) {
        char *str;
        if (i < 3)
            str = OutputPercent(ar[i]);
        else if (i == 3)
            str = OutputPercent(1.0f - ar[OUTPUT_WIN]);
        else
            str = OutputPercent(ar[i - 1]);

        while (*str == ' ')
            str++;

        pango_layout_set_text(layout, str, -1);
        gdk_draw_layout_with_colors(window, gc, cell_area->x + x, cell_area->y + y, layout, fg, 0);

        if (fOutputWinPC)
            x += _s_ZP;
        else
            x += _s_Z;

        if (i == 2) {
            int minOff = (_s_s - _s_m) / 2;
            if (fOutputWinPC)
                minOff -= _s_Y;
            pango_layout_set_text(layout, "-", -1);
            gdk_draw_layout_with_colors(window, gc, cell_area->x + x + minOff, cell_area->y + y, layout, fg, 0);

            x += _s_s;
        } else
            x += _s_Y * 2;
    }

    g_object_unref(layout);

    if (expose_area)
        gdk_gc_set_clip_rectangle(gc, NULL);
    g_object_unref(gc);
}
