#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
// #include <stdio.h>


// A plugin written to use lots deprecated api calls :)

#define PLUG_IN_PROC "plug-in-color-depth-reduction"
#define PLUG_IN_BINARY "color-depth-reduction"
#define PLUG_IN_ROLE "gimp-color-depth-reduction"

static void query(void);
static void run(const gchar * name,
        gint              nparams,
        const GimpParam * param,
        gint            * nreturn_vals,
        GimpParam      ** return_vals);

static gboolean dialog(GimpDrawable * drawable);
static void process(GimpDrawable * drawable, GimpPreview * preview);
static void ui_locked_channels_sync(GtkWidget * widget, gpointer callback_data);
static void ui_checkboxes_update(GtkToggleButton * widget, gpointer callback_data);

GimpPlugInInfo PLUG_IN_INFO = {
    NULL,
    NULL,
    query,
    run
};

typedef struct {
    gint red_depth;
    gint green_depth;
    gint blue_depth;
    gboolean clamp_lowest_bitval;
    gboolean lock_channels;
} pluginSettings;

// Startup defaults
static pluginSettings settings = {
    8,
    8,
    8,
    TRUE,
    FALSE
};

MAIN()

static void query(void) {
    static GimpParamDef args[] = {
        {
            GIMP_PDB_INT32,
            "run-mode",
            "Run mode"
        },
        {
            GIMP_PDB_IMAGE,
            "image",
            "Input image"
        },
        {
            GIMP_PDB_DRAWABLE,
            "drawable",
            "Input drawable"
        },
        {
            GIMP_PDB_INT32,
            "red",
            "Red channel bit depth (1-8)"
        },
        {
            GIMP_PDB_INT32,
            "green",
            "Green channel bit depth (1-8)"
        },
        {
            GIMP_PDB_INT32,
            "blue",
            "Blue channel bit depth (1-8)"
        }
    };

    gimp_install_procedure(PLUG_IN_PROC,
        "Reduce color depth per channel",
        "Allows interactive reduction of color depth for each RGB channel",
        "--",
        "--",
        "2023",
        "_Color Depth Reduction...",
        "RGB*",
        GIMP_PLUGIN,
        G_N_ELEMENTS(args), 0,
        args,
        NULL);

    gimp_plugin_menu_register(PLUG_IN_PROC, "<Image>/Filters/ColorDepth");
}

static void run(const gchar * name, gint nparams, const GimpParam * param, gint * nreturn_vals, GimpParam ** return_vals) {
    static GimpParam values[1];
    GimpPDBStatusType status = GIMP_PDB_SUCCESS;
    GimpRunMode run_mode;
    GimpDrawable * drawable;

    * nreturn_vals = 1;
    * return_vals = values;

    values[0].type = GIMP_PDB_STATUS;
    values[0].data.d_status = status;

    run_mode = param[0].data.d_int32;
    drawable = gimp_drawable_get(param[2].data.d_drawable);

    switch (run_mode) {
        case GIMP_RUN_INTERACTIVE:
            gimp_get_data(PLUG_IN_PROC, &settings);
            if (!dialog(drawable))
                return;
            break;

        case GIMP_RUN_NONINTERACTIVE:
            if (nparams != 6)
                status = GIMP_PDB_CALLING_ERROR;
            if (status == GIMP_PDB_SUCCESS) {
                settings.red_depth = CLAMP(param[3].data.d_int32, 1, 8);
                settings.green_depth = CLAMP(param[4].data.d_int32, 1, 8);
                settings.blue_depth = CLAMP(param[5].data.d_int32, 1, 8);
            }
            break;

        case GIMP_RUN_WITH_LAST_VALS:
            gimp_get_data(PLUG_IN_PROC, &settings);
            break;

        default:
            break;
    }

    if (status == GIMP_PDB_SUCCESS) {
        gimp_progress_init("Reducing color depth...");
        gimp_tile_cache_ntiles(2 * (drawable->width / gimp_tile_width() + 1));

        // Apply the previewed changes to the image
        process(drawable, NULL);

        // Try to trigger a redraw.... which only seems to redraw the bottom
        // of the image until another operation is performed...(such as undo, pencil, etc)
        gimp_drawable_detach(drawable);
        gimp_displays_flush();

        // Save settings if in interactive mode
        if (run_mode != GIMP_RUN_NONINTERACTIVE)
            gimp_set_data(PLUG_IN_PROC, &settings, sizeof(pluginSettings));
    }

    values[0].data.d_status = status;
}

    GtkAdjustment * adj_red, * adj_green, * adj_blue;
    GtkWidget * check_lock_channels, * check_clamp_lowest_bitval;

static gboolean dialog(GimpDrawable * drawable) {
    GtkWidget * dialog;
    GtkWidget * main_vbox;
    GtkWidget * preview;
    GtkWidget * table;
    GtkWidget * spin_red, * spin_green, * spin_blue;
    gboolean run;

    gimp_ui_init(PLUG_IN_BINARY, FALSE);

    dialog = gimp_dialog_new("Color Depth Reduction", PLUG_IN_ROLE,
        NULL, 0,
        gimp_standard_help_func, PLUG_IN_PROC,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        GTK_STOCK_OK, GTK_RESPONSE_OK,
        NULL);


    // Create UI elements

    main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_container_set_border_width(GTK_CONTAINER(main_vbox), 12);
    gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), main_vbox, TRUE, TRUE, 0);
    gtk_widget_show(main_vbox);

    preview = gimp_drawable_preview_new_from_drawable_id(drawable->drawable_id);
    //  preview = gimp_drawable_preview_new (drawable, NULL);
    gtk_box_pack_start(GTK_BOX(main_vbox), preview, TRUE, TRUE, 0);
    gtk_widget_show(preview);

    table = gtk_table_new(5, 2, FALSE);
    gtk_table_set_col_spacings(GTK_TABLE(table), 6);
    gtk_table_set_row_spacings(GTK_TABLE(table), 6);
    gtk_box_pack_start(GTK_BOX(main_vbox), table, FALSE, FALSE, 0);
    gtk_widget_show(table);

    adj_red = gtk_adjustment_new(settings.red_depth, 1, 8, 1, 1, 0);
    spin_red = gtk_spin_button_new(adj_red, 1.0, 0);
    gimp_table_attach_aligned(GTK_TABLE(table), 0, 0, "Red Channel Depth:", 0.0, 0.5, spin_red, 1, TRUE);

    adj_green = gtk_adjustment_new(settings.green_depth, 1, 8, 1, 1, 0);
    spin_green = gtk_spin_button_new(adj_green, 1.0, 0);
    gimp_table_attach_aligned(GTK_TABLE(table), 0, 1, "Green Channel Depth:", 0.0, 0.5, spin_green, 1, TRUE);

    adj_blue = gtk_adjustment_new(settings.blue_depth, 1, 8, 1, 1, 0);
    spin_blue = gtk_spin_button_new(adj_blue, 1.0, 0);
    gimp_table_attach_aligned(GTK_TABLE(table), 0, 2, "Blue Channel Depth:", 0.0, 0.5, spin_blue, 1, TRUE);

    check_lock_channels = gtk_check_button_new();
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_lock_channels), settings.lock_channels);
    gimp_table_attach_aligned(GTK_TABLE(table), 0, 3, "Lock Channels", 0.0, 0.5, check_lock_channels, 1, FALSE);
  
    check_clamp_lowest_bitval = gtk_check_button_new();
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_clamp_lowest_bitval), settings.clamp_lowest_bitval);
    gimp_table_attach_aligned(GTK_TABLE(table), 0, 4, "Clamp to Lowest Bit Value\n", 0.0, 0.5, check_clamp_lowest_bitval, 1, FALSE);
  

    // Set up UI event signals

    g_signal_connect_swapped(preview, "invalidated", G_CALLBACK(process), drawable);


    g_signal_connect(check_lock_channels,  "toggled", G_CALLBACK(ui_checkboxes_update), &settings.lock_channels);
    g_signal_connect(check_clamp_lowest_bitval, "toggled", G_CALLBACK(ui_checkboxes_update), &settings.clamp_lowest_bitval);

    // Read updated values in controls into settings vars
    g_signal_connect(adj_red,   "value-changed", G_CALLBACK(gimp_int_adjustment_update), &settings.red_depth);
    g_signal_connect(adj_green, "value-changed", G_CALLBACK(gimp_int_adjustment_update), &settings.green_depth);
    g_signal_connect(adj_blue,  "value-changed", G_CALLBACK(gimp_int_adjustment_update), &settings.blue_depth);

    // Lock channels together if needed
    g_signal_connect(adj_red,   "value-changed", G_CALLBACK(ui_locked_channels_sync), &settings.red_depth);
    g_signal_connect(adj_green, "value-changed", G_CALLBACK(ui_locked_channels_sync), &settings.green_depth);
    g_signal_connect(adj_blue,  "value-changed", G_CALLBACK(ui_locked_channels_sync), &settings.blue_depth);

    g_signal_connect_swapped(adj_red,   "value-changed", G_CALLBACK(gimp_preview_invalidate), preview);
    g_signal_connect_swapped(adj_green, "value-changed", G_CALLBACK(gimp_preview_invalidate), preview);
    g_signal_connect_swapped(adj_blue,  "value-changed", G_CALLBACK(gimp_preview_invalidate), preview);

    g_signal_connect_swapped(check_lock_channels,  "toggled", G_CALLBACK(gimp_preview_invalidate), preview);
    g_signal_connect_swapped(check_clamp_lowest_bitval, "toggled", G_CALLBACK(gimp_preview_invalidate), preview);

    gtk_widget_show(dialog);

    run = (gimp_dialog_run(GIMP_DIALOG(dialog)) == GTK_RESPONSE_OK);

    gtk_widget_destroy(dialog);

    return run;
}



static void ui_locked_channels_sync(GtkWidget * widget, gpointer callback_data) {

    // printf("in=%x, red=%x : ", callback_data, &settings.red_depth);
    // printf("  rgb(%3d,%3d,%3d ->", settings.red_depth, settings.green_depth, settings.blue_depth);
    if (settings.lock_channels) {
        // Update all other channels depending on which was clicked
        if (callback_data == &settings.red_depth) {
            settings.green_depth = settings.red_depth;
            settings.blue_depth = settings.red_depth;
            gtk_adjustment_set_value(adj_green, settings.red_depth);
            gtk_adjustment_set_value(adj_blue, settings.red_depth);
        }
        else if (callback_data == &settings.green_depth) {
            settings.red_depth = settings.green_depth;
            settings.blue_depth = settings.green_depth;
            gtk_adjustment_set_value(adj_red, settings.green_depth);
            gtk_adjustment_set_value(adj_blue, settings.green_depth);
        }
        else if (callback_data == &settings.blue_depth) {
            settings.red_depth = settings.blue_depth;
            settings.green_depth = settings.blue_depth;
            gtk_adjustment_set_value(adj_red, settings.blue_depth);
            gtk_adjustment_set_value(adj_green, settings.blue_depth);
        }
    }
    // printf("  rgb(%3d,%3d,%3d\n", settings.red_depth, settings.green_depth, settings.blue_depth);
}



static void ui_checkboxes_update(GtkToggleButton * widget, gpointer callback_data) {
    // Update settings for both checkboxes
    settings.lock_channels  = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_lock_channels));    
    settings.clamp_lowest_bitval = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_clamp_lowest_bitval));

    if (settings.lock_channels)
        ui_locked_channels_sync(adj_red, &settings.red_depth);
}



#define  TRUNCATE_BIT_DEPTH(value, bit_depth)  (value >> (8 - bit_depth))
// Spreads the reduced bit depth over the full 256 value range
// instead of clamping it to the minimum value for a given part of the range
// ex: 0..255 instead of 0..248
#define CLAMP_SPREAD_RANGE(value, bit_depth)   ((unsigned char)(255 * \
                                                 ((float)TRUNCATE_BIT_DEPTH(value, bit_depth) / (float)((1 << bit_depth) - 1)) ))

static void process(GimpDrawable * drawable, GimpPreview * preview) {
    gint x, y;
    gint width, height;
    GimpPixelRgn rgn_in, rgn_out;
    guchar * row;
    gint red_mask, green_mask, blue_mask;

    if (preview) {
        gimp_preview_get_position(preview, &x, &y);
        gimp_preview_get_size(preview, &width, &height);
    } else {
        gimp_drawable_mask_bounds(drawable->drawable_id, &x, &y, &width, &height);
        width -= x;
        height -= y;
    }

    row = g_new(guchar, width * drawable->bpp);

    // Source image data
    gimp_pixel_rgn_init( &rgn_in, drawable, x, y, width, height, FALSE, FALSE);

    // Output/Preview image data
    if (preview)
        gimp_pixel_rgn_init( &rgn_out, drawable, x, y, width, height, FALSE, TRUE);
    else
        gimp_pixel_rgn_init( &rgn_out, drawable, x, y, width, height, TRUE, TRUE);

    // This is a terrible hack
    //
    // Coerce GIMP to return the actual RGB values instead of mangled color profile
    // versions such as where RGB(0,0,0) returns RGB(13,13,13)
    //
    // "This One Weird Trick" is that merely requesting a GEGL buffer apparently causes GIMP
    // to start returning the actual unmodified pixel values without color/gamma correction.
    //
    // It doesn't seem to actually allocate the buffer (flushing with it fails, there are no
    // reported GEGL buffer leaks on app shutdown), so I guess we won't bother to free it.
    GeglBuffer * buffer = gimp_drawable_get_buffer(drawable);


    red_mask = ((1 << settings.red_depth) - 1) << (8 - settings.red_depth);
    green_mask = (1 << settings.green_depth) - 1 << (8 - settings.green_depth);
    blue_mask = (1 << settings.blue_depth) - 1 << (8 - settings.blue_depth);

    for (y = 0; y < height; y++) {
        gimp_pixel_rgn_get_row( &rgn_in, row, x, y, width);

        for (gint i = 0; i < width * drawable->bpp; i += drawable->bpp) {
            // Debug for checking whether gimp is mangling the rgb values with color/gamma correction
            //  if ((y==6) || (y == 15)) printf("%d: %d = %2d, %2d, %2d", y, i / drawable->bpp, 
            //      row[i], row[i+1], row[i+2]);

            if (settings.clamp_lowest_bitval) {
                // Clamp truncation style (rounds down to nearest bit depth boundary)
                row[i]     = (row[i]     & red_mask);
                row[i + 1] = (row[i + 1] & green_mask);
                row[i + 2] = (row[i + 2] & blue_mask);
            } else {
                // Clamp to utilize full range, resulting value is higher or lower
                // in bit depth band based on how bright or dim it is.
                row[i]     = CLAMP_SPREAD_RANGE(row[i    ], settings.red_depth);
                row[i + 1] = CLAMP_SPREAD_RANGE(row[i + 1], settings.green_depth);
                row[i + 2] = CLAMP_SPREAD_RANGE(row[i + 2], settings.blue_depth);
            }

            // if ((y==6) || (y == 15)) printf(" -> %2d, %2d, %2d\n",
            //     row[i], row[i+1], row[i+2]);
        }

        gimp_pixel_rgn_set_row( &rgn_out, row, x, y, width);
    }

    g_free(row);

    if (preview) {
        gimp_drawable_preview_draw_region(GIMP_DRAWABLE_PREVIEW(preview), &rgn_out);

    } else {
        gimp_drawable_flush(drawable);
        gimp_drawable_merge_shadow(drawable->drawable_id, TRUE);
        gimp_drawable_update(drawable->drawable_id, x, y, width, height);
        gimp_drawable_free_shadow(drawable->drawable_id);
    }
}
