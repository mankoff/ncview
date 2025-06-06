/*
 * Ncview by David W. Pierce.  A visual netCDF file viewer.
 * Copyright (C) 1993-2024 David W. Pierce
 *
 * This program  is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as 
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License, version 3, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * David W. Pierce
 * davidwilliampierce@gmail.com
 */

/*****************************************************************************
 *
 *	Elements which implement the X Windows user interface to ncview
 *
 *****************************************************************************/

#include "../ncview.includes.h"
#include "../ncview.defines.h"
#include "../ncview.protos.h"
#include "../ncview.bitmaps.h"
#include "fallback_resources.h"
#include <X11/CoreP.h>
#include <X11/CoreP.h>

#ifdef HAVE_PNG
#include <png.h>
#include <setjmp.h>
#endif

#define DEFAULT_BUTTON_WIDTH	55
#define DEFAULT_VARNAME_WIDTH	55
#define DEFAULT_LABEL_WIDTH	400
#define DEFAULT_DIMLABEL_WIDTH	95
#define DEFAULT_VARLABEL_WIDTH	95
#define DEFAULT_N_VARS_PER_ROW	4
#define DEFAULT_BLOWUP_SIZE	300
#define DEFAULT_VAR_COLORS	1
#define DEFAULT_AUTO_OVERLAY	1
#define CBAR_HEIGHT		24

#define XtNlabelWidth 		"labelWidth"
#define XtCLabelWidth		"LabelWidth"
#define XtNblowupDefaultSize 	"blowupDefaultSize"
#define XtCBlowupDefaultSize	"BlowupDefaultSize"
#define XtNdimLabelWidth 	"dimLabelWidth"
#define XtCDimLabelWidth 	"DimLabelWidth"
#define XtNvarLabelWidth 	"varLabelWidth"
#define XtCVarLabelWidth 	"VarLabelWidth"
#define XtNbuttonWidth 		"buttonWidth"
#define XtCButtonWidth		"ButtonWidth"
#define XtNvarnameWidth 	"varnameWidth"
#define XtCVarnameWidth		"VarnameWidth"
#define XtNnVarsPerRow		"nVarsPerRow"
#define XtCNVarsPerRow		"NVarsPerRow"
#define XtNdeltaStep		"deltaStep"
#define XtCDeltaStep		"DeltaStep"
#define XtNversion 		"version"
#define XtCVersion		"Version"
#define XtNvarcolors 		"varColors"
#define XtCvarcolors		"VarColors"
#define XtNautooverlay 		"autoOverlay"
#define XtCautooverlay		"AutoOverlay"
#define XtNforeground1d		"foreground1d"
#define XtCforeground1d		"Foreground1d"
#define XtNforeground2d		"foreground2d"
#define XtCforeground2d		"Foreground2d"
#define XtNforeground3d		"foreground3d"
#define XtCforeground3d		"Foreground3d"
#define XtNforeground4d		"foreground4d"
#define XtCforeground4d		"Foreground4d"
#define XtNforeground5d		"foreground5d"
#define XtCforeground5d		"Foreground5d"

/**************************/
extern 	NCVar	  *variables;
extern  Options	  options;
extern  ncv_pixel *pixel_transform;
/**************************/

typedef struct {
	int	label_width;		/* width of the informational labels */
	int	dimlabel_width;		/* as above, but for dimension labls */
	int	varlabel_width;		/* as above, but for variable labls */
	int	blowup_default_size;	/* default size, in pixels, of newly opened windows */
	int	button_width;		/* width of the control buttons */
	int	varname_width;		/* width of the variable name buttons */
	int	n_vars_per_row;		/* how many vars in one row before
					 * we start another.
					 */
	int	delta_step;		/* Becomes options.delta_step */
	float	version;		/* Must match compiled in version */
	int	var_colors;		/* 1 to use var colors, 0 not to */
	int	auto_overlay;		/* 1 to automatically put on overlays, 0 not to */
	Pixel	foreground1d;
	Pixel	foreground2d;
	Pixel	foreground3d;
	Pixel	foreground4d;
	Pixel	foreground5d;
} AppData, *AppDataPtr;

/* These are "global" to this directory, in the sense that other files
 * in this directory use them.
 */
Widget 		topLevel;
XtAppContext 	x_app_context;
Server_Info	server;

/* "Cmaplist" is MY list structure for holding all the colormaps. "colormap_list" is the "official"
 * list of known colormaps.  current_colormap_list is a time-saving pointer to the actual entry in 
 * the colormap_list list that is the currently loaded colormap.
 * "Colormap" is the standard X windows structure holding the colormap.  It is set in x_create_colormap()
 * Note these are "global" to this directory.
 */
Cmaplist		*colormap_list   = NULL, *current_colormap_list = NULL;
Colormap		current_colormap = (Colormap)NULL;

static AppData		app_data;
static XtIntervalId	timer;

static int		timer_enabled      = FALSE,
			ccontour_popped_up = FALSE,
			valid_display;

static float		default_version_number = 0.0;

static Pixmap		reverse_pixmap,
			backwards_pixmap,
			pause_pixmap,
			forward_pixmap,
			fastforward_pixmap;

/* Not static cuz used by set_options.c */
Pixmap 			open_circle_pixmap,
			closed_circle_pixmap;

static XEvent	event;

static Widget
	error_popup_widget = NULL,
		error_popupcanvas_widget,
			error_popupdialog_widget,
	dimsel_popup_widget,
		dimsel_popupcanvas_widget,
			dimsel_ok_button_widget,
			dimsel_cancel_button_widget,
	ccontourpanel_widget,
		ccontour_form_widget,
			ccontour_info1_widget,
				ccontour_viewport_widget,
					horiz_scroll_widget,
					vert_scroll_widget,
                		ccontour_widget,
			ccontour_info2_widget,
	commandcanvas_widget,
                buttonbox_widget,
			label1_widget,		/* typically title */
			label2_widget,		/* typically displayed variable name */
			label3_widget,		/* typically current scan location */
			label4_widget,		/* typically min, max info */
			label5_widget,		/* typically data value under cursor */
			label6_widget,		/* typically scalar dim info */
                        quit_button_widget,
			restart_button_widget,
                        reverse_button_widget,
                        backwards_button_widget,
                        pause_button_widget,
                        forward_button_widget,
                        fastforward_button_widget,
			edit_button_widget,
			info_button_widget,
			scrollspeed_label_widget,
			scrollspeed_widget,
			options_button_widget,
		optionbox_widget,
                        cmap_button_widget,
                        invert_button_widget,
                        invert_color_button_widget,
                        blowup_widget,
			transform_widget,
			dimset_widget,
			range_widget,
			blowup_type_widget,
			print_button_widget,
		colorbar_form_widget,
			colorbar_widget,
		varsel_form_widget,
			*var_selection_widget,	/* the boxes with N vars per box */
			varlist_label_widget,
			*varlist_widget,	/* The buttons that select a var */
			varsel_menu_widget,	/* Only if using menu-style var selection */
			var_menu_1d,		/* ditto */
			var_menu_2d,		/* ditto */
			var_menu_3d,		/* ditto */
			var_menu_4d,		/* ditto */
			var_menu_other,		/* ditto */
		labels_row_widget,
			lr_dim_widget,
			lr_name_widget,
			lr_min_widget,
			lr_cur_widget,
			lr_max_widget,
			lr_units_widget,
		*diminfo_row_widget = NULL,
			*diminfo_dim_widget = NULL,
			*diminfo_name_widget = NULL,
			*diminfo_min_widget = NULL,
			*diminfo_cur_widget = NULL,
			*diminfo_max_widget = NULL,
			*diminfo_units_widget = NULL,

		xdim_selection_widget,
			xdimlist_label_widget,
			*xdimlist_widget = NULL,

		ydim_selection_widget,
			ydimlist_label_widget,
			*ydimlist_widget = NULL;

static XtResource resources[] = {
    {
	XtNlabelWidth, 
	XtCLabelWidth,
	XtRInt,
	sizeof( int ),
	XtOffset( AppDataPtr, label_width ),
	XtRImmediate,
	(XtPointer)DEFAULT_LABEL_WIDTH,
    },
    {
	XtNblowupDefaultSize, 
	XtCBlowupDefaultSize,
	XtRInt,
	sizeof( int ),
	XtOffset( AppDataPtr, blowup_default_size ),
	XtRImmediate,
	(XtPointer)DEFAULT_BLOWUP_SIZE,
    },
    {
	XtNdimLabelWidth, 
	XtCDimLabelWidth,
	XtRInt,
	sizeof( int ),
	XtOffset( AppDataPtr, dimlabel_width ),
	XtRImmediate,
	(XtPointer)DEFAULT_DIMLABEL_WIDTH,
    },
    {
	XtNvarLabelWidth, 
	XtCVarLabelWidth,
	XtRInt,
	sizeof( int ),
	XtOffset( AppDataPtr, varlabel_width ),
	XtRImmediate,
	(XtPointer)DEFAULT_VARLABEL_WIDTH,
    },
    {
	XtNbuttonWidth, 
	XtCButtonWidth,
	XtRInt,
	sizeof( int ),
	XtOffset( AppDataPtr, button_width ),
	XtRImmediate,
	(XtPointer)DEFAULT_BUTTON_WIDTH,
    },
    {
	XtNvarnameWidth, 
	XtCVarnameWidth,
	XtRInt,
	sizeof( int ),
	XtOffset( AppDataPtr, varname_width ),
	XtRImmediate,
	(XtPointer)DEFAULT_VARNAME_WIDTH,
    },
    {
	XtNnVarsPerRow, 
	XtCNVarsPerRow,
	XtRInt,
	sizeof( int ),
	XtOffset( AppDataPtr, n_vars_per_row ),
	XtRImmediate,
	(XtPointer)DEFAULT_N_VARS_PER_ROW,
    },
    {
	XtNdeltaStep,
	XtCDeltaStep,
	XtRInt,
	sizeof( int ),
	XtOffset( AppDataPtr, delta_step ),
	XtRImmediate,
	(XtPointer)DEFAULT_DELTA_STEP,	/* see file do_buttons.c for interpretation of this */
    },
    {
	XtNversion,
	XtCVersion,
	XtRFloat,
	sizeof( float ),
	XtOffset( AppDataPtr, version ),
	XtRFloat,
	(XtPointer)&default_version_number
    },
    {
	XtNvarcolors,
	XtCvarcolors,
	XtRInt,
	sizeof( int ),
	XtOffset( AppDataPtr, var_colors ),
	XtRImmediate,
	(XtPointer)DEFAULT_VAR_COLORS,
    },
    {
	XtNautooverlay,
	XtCautooverlay,
	XtRInt,
	sizeof( int ),
	XtOffset( AppDataPtr, auto_overlay ),
	XtRImmediate,
	(XtPointer)DEFAULT_AUTO_OVERLAY,
    },
    {
	XtNforeground1d,
	XtCforeground1d,
	XtRPixel,
	sizeof( Pixel ),
	XtOffset( AppDataPtr, foreground1d ),
	XtRString,
	"grey"
    },
    {
	XtNforeground2d,
	XtCforeground2d,
	XtRPixel,
	sizeof( Pixel ),
	XtOffset( AppDataPtr, foreground2d ),
	XtRString,
	"blue2"
    },
    {
	XtNforeground3d,
	XtCforeground3d,
	XtRPixel,
	sizeof( Pixel ),
	XtOffset( AppDataPtr, foreground3d ),
	XtRString,
	"yellow"
    },
    {
	XtNforeground4d,
	XtCforeground4d,
	XtRPixel,
	sizeof( Pixel ),
	XtOffset( AppDataPtr, foreground4d ),
	XtRString,
	"pink"
    },
    {
	XtNforeground5d,
	XtCforeground5d,
	XtRPixel,
	sizeof( Pixel ),
	XtOffset( AppDataPtr, foreground5d ),
	XtRString,
	"white"
    },
};

static int	error_popup_done    = FALSE, error_popup_result    = 0;
static int	dimsel_popup_done   = FALSE, dimsel_popup_result   = 0;
static Cursor	busy_cursor;

/* We need to be able to find all the widgets in the variable selection menus, in order
 * to set those widgets' sensitivity.  When a list is being used, the varlist_widget
 * array serves this function.  When menus are being used, the following array does this.
 */
static Widget	*varsel_menu_widget_list;

/******************************************************************************
 * These are only used in this file
 */
unsigned char interp( int i, int range_i, unsigned char *mat, int n_entries );
void	x_init_widgets		( Widget top );
void	x_init_pixmaps		( Widget top );
void 	create_pixmap 		( Widget shell_widget, Pixmap *pixmap, int id );
void	x_set_lab     		( Widget w, char *s, int width );
void 	x_add_to_cmap_list	( char *name, Colormap new_colormap );
void	colormap_back		( Widget w, XButtonEvent *e, String *p, Cardinal *n );
void 	x_make_dim_button_util  ( int dimension, Stringlist *dim_list, char *prefix, 
					Widget **dimlist_widget, 
					Widget parent_widget, 
					char *selected_name );
void	x_popup			( char *message );
int	x_dialog                ( char *message, char *ret_string, int want_cancel_button );
void	track_pointer		( void );

/* the button callbacks and actions in x_interface.c */
/* _mod1 is a standard callback; _mod2 is an accelerated action, and _mod3
 * is a backwards action */
void 	print_button_callback(Widget w, XtPointer client_data, XtPointer call_data );
void 	blowup_type_mod1(Widget w, XtPointer client_data, XtPointer call_data );
void 	varsel_menu_select(Widget w, XtPointer client_data, XtPointer call_data );
void 	range_mod1	(Widget w, XtPointer client_data, XtPointer call_data );
void 	options_mod1	(Widget w, XtPointer client_data, XtPointer call_data);
void 	range_mod3	(Widget w, XButtonEvent *e, String *p, Cardinal *n );
void 	dimset_mod1	(Widget w, XtPointer client_data, XtPointer call_data );
void 	quit_mod1	(Widget w, XtPointer client_data, XtPointer call_data );
void 	cmap_mod1	(Widget w, XtPointer client_data, XtPointer call_data );
void 	cmap_mod3	(Widget w, XButtonEvent *e, String *p, Cardinal *n );
void 	restart_mod1	(Widget w, XtPointer client_data, XtPointer call_data );
void 	reverse_mod1	(Widget w, XtPointer client_data, XtPointer call_data );
void 	reverse_mod2   	(Widget w, XButtonEvent *e, String *p, Cardinal *n );
void 	back_mod1	(Widget w, XtPointer client_data, XtPointer call_data );
void 	back_mod2    	(Widget w, XButtonEvent *e, String *p, Cardinal *n );
void 	pause_mod1	(Widget w, XtPointer client_data, XtPointer call_data );
void 	forward_mod1	(Widget w, XtPointer client_data, XtPointer call_data );
void 	forward_mod2 	(Widget w, XButtonEvent *e, String *p, Cardinal *n );
void 	ff_mod1		(Widget w, XtPointer client_data, XtPointer call_data );
void 	edit_mod1	(Widget w, XtPointer client_data, XtPointer call_data );
void 	info_mod1	(Widget w, XtPointer client_data, XtPointer call_data );
void 	fastforward_mod2 (Widget w, XButtonEvent *e, String *p, Cardinal *n );
void 	invert_mod1	(Widget w, XtPointer client_data, XtPointer call_data );
void 	invert_color_mod1 (Widget w, XtPointer client_data, XtPointer call_data );
void 	blowup_mod1	(Widget w, XtPointer client_data, XtPointer call_data );
void 	blowup_mod2	(Widget w, XButtonEvent *e, String *p, Cardinal *n );
void 	blowup_mod3	(Widget w, XButtonEvent *e, String *p, Cardinal *n );
void 	blowup_mod4	(Widget w, XButtonEvent *e, String *p, Cardinal *n );
void 	varlist_mod1	(Widget w, XtPointer client_data, XtPointer call_data );
void 	xdimlist_mod1	(Widget w, XtPointer client_data, XtPointer call_data );
void 	ydimlist_mod1	(Widget w, XtPointer client_data, XtPointer call_data );
void 	transform_mod1	(Widget w, XtPointer client_data, XtPointer call_data );
void 	dimsel_callback	(Widget w, XtPointer client_data, XtPointer call_data );
void 	error_popup_callback(Widget w, XtPointer client_data, XtPointer call_data );
void 	diminfo_cur_mod1(Widget w, XtPointer client_data, XtPointer call_data);
void 	diminfo_cur_mod2(Widget w, XButtonEvent *e, String *p, Cardinal *n );
void 	diminfo_cur_mod3(Widget w, XButtonEvent *e, String *p, Cardinal *n );
void 	diminfo_cur_mod4(Widget w, XButtonEvent *e, String *p, Cardinal *n );
void 	diminfo_min_mod1(Widget w, XtPointer client_data, XtPointer call_data);
void 	do_plot_xy	(Widget w, XButtonEvent *e, String *p, Cardinal *n );
void 	do_quit_app            (Widget w, XButtonEvent *e, String *p, Cardinal *n );
void 	do_set_dataedit_place  (Widget w, XButtonEvent *e, String *p, Cardinal *n );
void 	do_set_min_from_curdata(Widget w, XButtonEvent *e, String *p, Cardinal *n );
void 	do_set_max_from_curdata(Widget w, XButtonEvent *e, String *p, Cardinal *n );
void 	expose_ccontour();
void 	expose_colorbar();

void 	testf(Widget w, XButtonEvent *e, String *p, Cardinal *n );

static void 	add_callbacks( void );

#ifdef HAVE_PNG
static void 	dump_to_png( unsigned char *data, size_t width, size_t height,
				size_t timestep );
#endif

/*************************************************************************************************/
void x_parse_args( int *p_argc, char **argv )
{
	Visual	*visual;
	char	program_title[132];
	static XImage	*ximage;
	long	data;
	int	debug;
	Screen	*screen;
	Display	*display;

	topLevel =  XtVaAppInitialize(
		&x_app_context,	 	/* Application context           */
		"Ncview",	 	/* Application class             */
		NULL, 0,	 	/* command line option list      */
		p_argc, argv,	 	/* command line args             */
		fallback_resources,	/* for missing app-defaults file */
		NULL );		 	/* terminate varargs list        */
	
	snprintf( program_title, 130, "Ncview %s\n", PROGRAM_VERSION_STRING );
	XtVaSetValues( topLevel, XtNtitle, program_title, NULL );

	debug = 0;

	display = XtDisplay( topLevel );
	screen  = XtScreen ( topLevel );	/* Note: a pointer to Screen struct, NOT the integer screen number (Use DefaultScreen(display) for that) */

	/* Have to determine the visual class as soon as possible!
	 * In particular, must be done BEFORE x_initialize, because
	 * the colormap initialization is called before x_initialize,
	 * and we have to know the display class to init the colormaps.
	 */
	visual = XDefaultVisualOfScreen( screen );
	switch ( visual->class ) {
		case PseudoColor:	
			if(debug) printf( "visual class: PseudoColor\n" );
			options.display_type = PseudoColor;
			valid_display = TRUE;
			break;

		case TrueColor:
			if(debug) printf( "visual class: TrueColor\n" );
			options.display_type = TrueColor;
			valid_display = TRUE;
			break;

		case StaticColor:
			if(debug) printf( "visual class: StaticColor\n" );
			fprintf( stderr, "Sorry, StaticColor displays ");
			fprintf( stderr, "are not supported.\n" );
			options.display_type = StaticColor;
			valid_display = FALSE;
			break;

		case DirectColor:
			if(debug) printf( "visual class: DirectColor\n" );
			fprintf( stderr, "Sorry, DirectColor displays ");
			fprintf( stderr, "are not supported.\n" );
			valid_display = FALSE;
			break;

		case GrayScale:
			if(debug) printf( "visual class: GrayScale\n" );
			fprintf( stderr, "Sorry, GrayScale displays ");
			fprintf( stderr, "are not supported.\n" );
			valid_display = FALSE;
			break;

		case StaticGray:
			if(debug) printf( "visual class: StaticGrey\n" );
			fprintf( stderr, "Sorry, StaticGray displays ");
			fprintf( stderr, "are not supported.\n" );
			valid_display = FALSE;
			break;
		
		default:
			fprintf( stderr, "ERROR!! Unknown visual class %d!!\n",
				visual->class );
			exit( -1 );
		}

	/* Make a test image to get our server parameters
	*/
	data = 0;       /* fake test data */
	if( options.display_type == TrueColor ) {
		ximage  = XCreateImage(
			display,
			XDefaultVisualOfScreen( screen ),
			XDefaultDepthOfScreen ( screen ),
			ZPixmap,
			0,
			(char *)data, 
			1, 1, 32, 0 );
		}
	else /* display_type == PseudoColor */
		{
		ximage  = XCreateImage(
			display,
			XDefaultVisualOfScreen( screen ),
			XDefaultDepthOfScreen ( screen ),
			ZPixmap,
			0,
			(char *)data,
			1, 1, 8, 0 );
		}

	if( debug ) {
		printf( "byte order: %s\n", 
			(ximage->byte_order == LSBFirst) ? "LSBFirst" : "MSBFirst" );
		printf( "bitmap unit: %d\n", ximage->bitmap_unit );
		printf( "bit order: %s\n", 
			(ximage->bitmap_bit_order == LSBFirst) ? "LSBFirst" : "MSBFirst" );
		printf( "bitmap pad: %d\n", ximage->bitmap_pad );
		printf( "depth: %d\n", ximage->depth );
		printf( "bytes per line: %d\n", ximage->bytes_per_line );
		printf( "bits per pixel: %d\n", ximage->bits_per_pixel );
		printf( "r, g, b masks: %0lx, %0lx, %0lx\n", ximage->red_mask,
				ximage->green_mask, ximage->blue_mask );
		printf( "Server root window geometry: %d x %d\n", 
				DisplayWidth ( display, DefaultScreen(display) ),
				DisplayHeight( display, DefaultScreen(display) ));
		}

	server.width 		= DisplayWidth ( display, DefaultScreen(display) );
	server.height		= DisplayHeight( display, DefaultScreen(display) );
	server.byte_order	= ximage->byte_order;
	server.bits_per_pixel  	= ximage->bits_per_pixel;
	server.bytes_per_pixel 	= server.bits_per_pixel/8;
	server.bitmap_unit     	= ximage->bitmap_unit;
	server.bitmap_pad      	= ximage->bitmap_pad;
	server.depth           	= ximage->depth;
	if( server.byte_order == MSBFirst ) {
		if( ximage->red_mask > ximage->blue_mask )
			server.rgb_order = ORDER_BGR;
		else
			server.rgb_order = ORDER_RGB;
		}
	else
		{
		if( ximage->red_mask > ximage->blue_mask )
			server.rgb_order = ORDER_RGB;
		else
			server.rgb_order = ORDER_BGR;
		}

	/* These are for 16-bit displays.  These numbers are surely
	 * wrong for other displays than the one 16-bit display I
	 * had to test them on, which was an Intel Linux box.
	 */
	server.shift_blue = 11;
	server.shift_red  = 8;
	server.shift_green_upper = 13;
	server.shift_green_lower = 5;

	server.mask_red = 0x00f8;
	server.mask_green_upper = 0x0007; /* goes with red */
	server.mask_green_lower = 0x00e0; /* goes with blue */
	server.mask_blue = 0x001f;
}

/*************************************************************************************************/
void x_initialize()
{
	void	  check_app_res();
	void 	  track_pointer();

	static XtActionsRec new_actions[] = {
		{"cmap_mod3",    	(XtActionProc)cmap_mod3        	},
		{"reverse_mod2",     	(XtActionProc)reverse_mod2     	},
		{"back_mod2",     	(XtActionProc)back_mod2        	},
		{"forward_mod2",	(XtActionProc)forward_mod2	},
		{"fastforward_mod2",	(XtActionProc)fastforward_mod2	},
		{"diminfo_cur_mod2",	(XtActionProc)diminfo_cur_mod2  },
		{"diminfo_cur_mod3",	(XtActionProc)diminfo_cur_mod3  },
		{"diminfo_cur_mod4",	(XtActionProc)diminfo_cur_mod4  },
		{"range_mod3",		(XtActionProc)range_mod3	},
		{"blowup_mod2",		(XtActionProc)blowup_mod2	},
		{"blowup_mod3",		(XtActionProc)blowup_mod3	},
		{"blowup_mod4",		(XtActionProc)blowup_mod4	},
		{"do_plot_xy",		(XtActionProc)do_plot_xy	},
		{"testf",		(XtActionProc)testf		},
		{"do_quit_app",            (XtActionProc)do_quit_app             },
		{"do_set_dataedit_place",  (XtActionProc)do_set_dataedit_place   },
		{"do_set_min_from_curdata",(XtActionProc)do_set_min_from_curdata },
		{"do_set_max_from_curdata",(XtActionProc)do_set_max_from_curdata },
	};

	XtVaGetApplicationResources( topLevel,
		&app_data,
		resources,
		XtNumber(resources),
		NULL );
	check_app_res( &app_data );
	/* This shouldn't be in this file, really, but X can get it from
	 * the resource file, and there is no point in having ncview read
	 * in two different configuration files.
	 */
	options.delta_step          = app_data.delta_step;
	options.blowup_default_size = app_data.blowup_default_size;

	XtAppAddActions( x_app_context, new_actions, XtNumber( new_actions ));
	x_init_pixmaps( topLevel );
	x_init_widgets( topLevel );
	add_callbacks ();

	x_range_init();
	x_plot_range_init();

	XtRealizeWidget( topLevel );

	busy_cursor = XCreateFontCursor( 
		XtDisplay( commandcanvas_widget ), XC_watch );
	if( busy_cursor == BadValue )
		busy_cursor = (Cursor)NULL;

	
	if( options.display_type == PseudoColor ) {
		XSetWindowColormap( XtDisplay(topLevel), XtWindow(topLevel), 
				current_colormap );
		}

	/* Add the event handler which tracks the cursor in
	 * the data window.
	 */
	XtAddEventHandler( ccontour_widget, 
		PointerMotionMask | ButtonPressMask,
		False,
		(XtEventHandler)track_pointer,
		NULL );

	plot_xy_init();

	x_display_info_init();

	set_options_init();

	cbar_init();
}

/*************************************************************************************************/
void x_query_pointer_position( int *ret_x, int *ret_y )
{
	Window root_return, child_window_return;
	int	root_x, root_y, win_x, win_y;
	unsigned int keys_and_buttons;

	if( XQueryPointer( XtDisplay(topLevel),
			XtWindow( ccontour_widget ),
			&root_return,
			&child_window_return,
			&root_x,
			&root_y,
			&win_x,
			&win_y,
			&keys_and_buttons ) ) {
		*ret_x = win_x;
		*ret_y = win_y;
		}
	else
		{
		*ret_x = -1;
		*ret_y = -1;
		}
}

/*************************************************************************************************/
void track_pointer()
{
	Window root_return, child_window_return;
	int	root_x, root_y, win_x, win_y;
	unsigned int keys_and_buttons;

	if( XQueryPointer( XtDisplay(topLevel),
			XtWindow( ccontour_widget ),
			&root_return,
			&child_window_return,
			&root_x,
			&root_y,
			&win_x,
			&win_y,
			&keys_and_buttons ) )
		report_position( win_x, win_y, keys_and_buttons );
}

/*************************************************************************************************/
void x_init_widgets_ccontourpanel( Widget top )
{
	if( options.display_type == PseudoColor ) {
		ccontourpanel_widget = XtVaCreatePopupShell(
			options.window_title,
			transientShellWidgetClass,
			top,
			XtNcolormap, current_colormap,
			NULL );

		ccontour_form_widget = XtVaCreateManagedWidget(
			"ccontourform",
			boxWidgetClass,
			ccontourpanel_widget,
			XtNorientation, XtorientVertical,
			XtNhSpace, 0,
			XtNvSpace, 0,
			XtNcolormap, current_colormap,
			NULL);
		}
	else
		{
		ccontourpanel_widget = XtVaCreatePopupShell(
			options.window_title,
			transientShellWidgetClass,
			top,
			NULL );

		ccontour_form_widget = XtVaCreateManagedWidget(
			"ccontourform",
			boxWidgetClass,
			ccontourpanel_widget,
			XtNorientation, XtorientVertical,
			XtNhSpace, 0,
			XtNvSpace, 0,
			NULL);
		}

	if( options.want_extra_info )
		ccontour_info1_widget = XtVaCreateManagedWidget(
			"ccontourinfo1",
			labelWidgetClass,
			ccontour_form_widget,
			XtNlabel, "no variable selected - 1",
			XtNborderWidth, 0,
			XtNwidth, 300,
			NULL );

	ccontour_viewport_widget = XtVaCreateManagedWidget(
		"ccontour_viewport",
		viewportWidgetClass,
		ccontour_form_widget,
		XtNwidth, 400,
		XtNheight, 300,
		XtNallowHoriz, True,
		XtNallowVert,  True,
		NULL );
		/*XtNforceBars, True,*/

	ccontour_widget = XtVaCreateManagedWidget(
		"ccontour",
		simpleWidgetClass,
		ccontour_viewport_widget,
/* cursorName is only present in R5 release */
#ifdef XtNcursorName
/*			XtNcursorName, "dotbox", */
#endif
		XtNwidth,  500,
		XtNheight, 300,
/*			XtNdepth,  server.depth, */
		NULL);

	ccontour_widget->core.widget_class->core_class.compress_exposure = TRUE;
	XtAddEventHandler( ccontour_widget, ExposureMask, FALSE,
		(XtEventHandler)expose_ccontour, NULL );

	if( options.want_extra_info )
		ccontour_info2_widget = XtVaCreateManagedWidget(
			"ccontourinfo2",
			labelWidgetClass,
			ccontour_form_widget,
			XtNlabel, "no variable selected - 2",
			XtNwidth, 300,
			XtNfromVert, ccontour_widget,
			XtNborderWidth, 0,
			NULL );

	XtAugmentTranslations( ccontour_widget,
		XtParseTranslationTable( 
			"#augment\n\
			Ctrl<Btn1Up>: do_set_min_from_curdata()\n\
			<Btn1Up>: do_plot_xy()\n\
			Ctrl<Btn1Motion>: do_set_min_from_curdata()\n\
			Ctrl<Btn3Up>: do_set_max_from_curdata()\n\
			Ctrl<Btn3Motion>: do_set_max_from_curdata()\n\
			<Key>q: do_quit_app()\n\
			<Btn2Up>: do_set_dataedit_place()\n\
			<Btn2Motion>: do_set_dataedit_place()" ));
}
	
/*************************************************************************************************/
void x_init_widgets_labels( Widget parent )
{
	/* Title */
	label1_widget = XtVaCreateManagedWidget(
		"label1",
		labelWidgetClass,
		parent,
		XtNlabel, "no variable selected",
		XtNwidth, app_data.label_width,
		NULL );

	/* Variable name */
	label2_widget = XtVaCreateManagedWidget(
		"label2",
		labelWidgetClass,
		parent,
		XtNlabel, PROGRAM_ID,
		XtNwidth, app_data.label_width,
		XtNfromVert, label1_widget,
		NULL);

	/* Frame number */
	label3_widget = XtVaCreateManagedWidget(
		"label3",
		labelWidgetClass,
		parent,
		XtNwidth, app_data.label_width,
		XtNfromVert, label2_widget,
		XtNlabel, "*** SELECT A VARIABLE TO START ***",
		NULL);

	/* Displayed range */
	label4_widget = XtVaCreateManagedWidget(
		"label4",
		labelWidgetClass,
		parent,
		XtNlabel, "",
		XtNfromVert, label3_widget,
		XtNwidth, app_data.label_width,
		NULL);

	/* Current location and value */
	label5_widget = XtVaCreateManagedWidget(
		"label5",
		labelWidgetClass,
		parent,
		XtNlabel, "",
		XtNfromVert, label4_widget,
		XtNwidth, app_data.label_width,
		NULL);

	label6_widget = XtVaCreateManagedWidget(
		"label6",
		labelWidgetClass,
		parent,
		XtNlabel, "",
		XtNfromVert, label5_widget,
		XtNwidth, app_data.label_width,
		XtNjustify, XtJustifyLeft,
		XtNborderWidth, 0,
		NULL);
 
}

/*************************************************************************************************/
void x_init_widgets_buttonbox( Widget parent )
{
	buttonbox_widget = XtVaCreateManagedWidget(
		"buttonbox",
		boxWidgetClass,
		parent,
		XtNorientation, XtorientHorizontal,
		XtNfromVert, label6_widget,
		NULL);

	/* Quit */
	quit_button_widget = XtVaCreateManagedWidget(
		"quit",
		commandWidgetClass,
		buttonbox_widget,
		XtNheight, 24,
		XtNwidth, app_data.button_width,
		NULL);

	/*  ->1  */
	restart_button_widget = XtVaCreateManagedWidget(
		"restart",
		commandWidgetClass,
		buttonbox_widget,
		XtNheight, 24,
		XtNsensitive, False,
		XtNlabel, "->1",
		XtNwidth, app_data.button_width,
		NULL);

	/*   <<   */
	reverse_button_widget = XtVaCreateManagedWidget(
		"reverse",
		toggleWidgetClass,
		buttonbox_widget,
		XtNbitmap, reverse_pixmap,
		XtNsensitive, False,
		NULL);
	XtOverrideTranslations( reverse_button_widget,
		XtParseTranslationTable( 
			"#override Ctrl<Btn1Down>,<Btn1Up>: reverse_mod2()" ));

	/*  <  */
	backwards_button_widget = XtVaCreateManagedWidget(
		"back",
		commandWidgetClass,
		buttonbox_widget,
		XtNbitmap, backwards_pixmap,
		XtNsensitive, False,
		NULL);
	XtOverrideTranslations( backwards_button_widget,
		XtParseTranslationTable( 
			"#override Ctrl<Btn1Down>,<Btn1Up>: back_mod2()" ));

	/*   ||   */
	pause_button_widget = XtVaCreateManagedWidget(
		"pause",
		toggleWidgetClass,
		buttonbox_widget,
		XtNbitmap, pause_pixmap,
		XtNradioGroup, reverse_button_widget,
		XtNstate, True,
		XtNsensitive, False,
		NULL);

	/*   >   */
	forward_button_widget = XtVaCreateManagedWidget(
		"forward",
		commandWidgetClass,
		buttonbox_widget,
		XtNbitmap, forward_pixmap,
		XtNradioGroup, reverse_button_widget,
		XtNsensitive, False,
		NULL);
	XtOverrideTranslations( forward_button_widget,
		XtParseTranslationTable( 
			"#override Ctrl<Btn1Down>,<Btn1Up>: forward_mod2()" ));

	/*   >>   */
	fastforward_button_widget = XtVaCreateManagedWidget(
		"fastforward",
		toggleWidgetClass,
		buttonbox_widget,
		XtNbitmap, fastforward_pixmap,
		XtNradioGroup, reverse_button_widget,
		XtNsensitive, False,
		NULL);
	XtOverrideTranslations( fastforward_button_widget,
		XtParseTranslationTable( 
			"#override Ctrl<Btn1Down>,<Btn1Up>: fastforward_mod2()" ));

	/*   Edit   */
	edit_button_widget = XtVaCreateManagedWidget(
		"Edit",
		commandWidgetClass,
		buttonbox_widget,
		XtNheight, 24,
		XtNsensitive, False,
		NULL);

	/*   ?  */
	info_button_widget = XtVaCreateManagedWidget(
		"?",
		commandWidgetClass,
		buttonbox_widget,
		XtNheight, 24,
		XtNsensitive, False,
		NULL);

	scrollspeed_label_widget = XtVaCreateManagedWidget(
		"Delay:",
		commandWidgetClass,
		buttonbox_widget,
		XtNheight, 24,
		XtNwidth, app_data.button_width,
		XtNborderWidth, 0,
		NULL);

	scrollspeed_widget = XtVaCreateManagedWidget(
		"scrollspeed",
		scrollbarWidgetClass,
		buttonbox_widget,
		XtNorientation, XtorientHorizontal,
		/* XtNshown, 0.1, */
		XtNthumb, None,
		XtNlength, 75,
		XtNthickness, 24,
		NULL);
	XtOverrideTranslations( scrollspeed_widget,
		XtParseTranslationTable( 
			"<Btn1Down>:StartScroll(Continuous) MoveThumb() NotifyThumb() \n\
			<Btn1Motion>:MoveThumb() NotifyThumb()" ));

	options_button_widget = XtVaCreateManagedWidget(
		"Opts",
		commandWidgetClass,
		buttonbox_widget,
		XtNheight, 24,
		XtNsensitive, True,
		NULL);
}
	
/*************************************************************************************************/
void x_init_widgets_optionbox( Widget parent )
{
	optionbox_widget = XtVaCreateManagedWidget(
		"optionbox",
		boxWidgetClass,
		parent,
		XtNorientation, XtorientHorizontal,
		XtNfromVert, buttonbox_widget,
		NULL);

	cmap_button_widget = XtVaCreateManagedWidget(
		"cmap",
		commandWidgetClass,
		optionbox_widget,
		XtNlabel, current_colormap_list->name,
		XtNsensitive, False,
		XtNwidth, app_data.button_width,
		NULL);
	XtAugmentTranslations( cmap_button_widget,
		XtParseTranslationTable( 
			"<Btn3Down>,<Btn3Up>: cmap_mod3()" ));  /* for when the user RIGHT clicks on the button */

	invert_button_widget = XtVaCreateManagedWidget(
		"Inv P",
		toggleWidgetClass,
		optionbox_widget,
		XtNsensitive, False,
		XtNwidth, app_data.button_width,
		NULL);

	invert_color_button_widget = XtVaCreateManagedWidget(
		"Inv C",
		toggleWidgetClass,
		optionbox_widget,
		XtNsensitive, False,
		XtNwidth, app_data.button_width,
		NULL);

	blowup_widget = XtVaCreateManagedWidget(
		"blowup",
		commandWidgetClass,
		optionbox_widget,
		XtNsensitive, False,
		XtNwidth, app_data.button_width,
		XtNlabel, "Mag X1",
		NULL);
	XtOverrideTranslations( blowup_widget,
		XtParseTranslationTable( 
			"Ctrl<Btn3Down>,<Btn3Up>: blowup_mod4()\n\
			<Btn3Down>,<Btn3Up>: blowup_mod3()\n\
			Ctrl<Btn1Down>,<Btn1Up>: blowup_mod2()" ));

	transform_widget = XtVaCreateManagedWidget(
		"transform",
		commandWidgetClass,
		optionbox_widget,
		XtNsensitive, True,
		XtNlabel, "Linear",
		XtNwidth, app_data.button_width,
		XtNsensitive, False,
		NULL);

	dimset_widget = XtVaCreateManagedWidget(
		"dimset",
		commandWidgetClass,
		optionbox_widget,
		XtNsensitive, False,
		XtNlabel, "Axes",
		XtNwidth, app_data.button_width,
		NULL);

	range_widget = XtVaCreateManagedWidget(
		"Range",
		commandWidgetClass,
		optionbox_widget,
		XtNsensitive, False,
		XtNwidth, app_data.button_width,
		NULL);
	XtAugmentTranslations( range_widget,
		XtParseTranslationTable( 
			"<Btn3Down>,<Btn3Up>: range_mod3()" ));

	blowup_type_widget = XtVaCreateManagedWidget(
		"blowup_type",
		commandWidgetClass,
		optionbox_widget,
		XtNsensitive, False,
		XtNwidth, app_data.button_width,
		NULL);

	print_button_widget = XtVaCreateManagedWidget(
		"Print",
		commandWidgetClass,
		optionbox_widget,
		XtNsensitive, False,
		XtNwidth, app_data.button_width,
		NULL);
}

/*************************************************************************************************
 * If place2insert == 0, then new value will go at head of array vlist; if
 * place2insert == 1, new value will go into second place; etc.
 */
void insert_into_array( long *vlist, int nlist, int val2insert, int place2insert )
{
	int i, n_move_down;

	n_move_down = nlist - place2insert;

	for( i=0; i<n_move_down; i++ ) 
		vlist[nlist-i] = vlist[nlist-i-1];

	vlist[place2insert] = val2insert;
}

/*************************************************************************************************/
/* 'nlist' is the length of vlist, i.e., the number of variables already in the list; we are 
 * adding the name in array 'names' that has index idx: names[idx]
 */
void insert_alpha_sorted( NCVar *v, long *vlist, int nlist, int idx, char **names )
{
	int i;

	if( nlist == 0 ) {
		vlist[nlist] = idx;
		return;
		}

	for( i=0; i<nlist; i++ ) {
		if( strcmp( names[idx], names[ vlist[i] ] ) < 0 ) {
			insert_into_array( vlist, nlist, idx, i );
			return;
			}
		}

	/* Does not come BEFORE anything in array, so append at end */
	vlist[nlist] = idx;
}

/*************************************************************************************************/
void x_sort_vars_by_ndims( NCVar *v, long **vl_1d, int *n_1d, long **vl_2d, int *n_2d, long **vl_3d, int *n_3d,
		long **vl_4d, int *n_4d, long **vl_other, int *n_other )
{
	NCVar *cursor;
	int   nmax, i;
	char  **names;

	*n_1d = 0;
	*n_2d = 0;
	*n_3d = 0;
	*n_4d = 0;
	*n_other = 0;

	nmax = n_vars_in_list( v );
	
	(*vl_1d)    = (long *)malloc( nmax * sizeof( long ));
	(*vl_2d)    = (long *)malloc( nmax * sizeof( long ));
	(*vl_3d)    = (long *)malloc( nmax * sizeof( long ));
	(*vl_4d)    = (long *)malloc( nmax * sizeof( long ));
	(*vl_other) = (long *)malloc( nmax * sizeof( long ));

	/* Get names so we can alpha sort them */
	cursor = v;
	names  = (char **)malloc( nmax * sizeof( char * ));
	for( i=0; i<nmax; i++ ) {
		names[i] = cursor->name;
		cursor   = cursor->next;
		}

	cursor = v;
	for( i=0; i<nmax; i++ ) {
		switch( cursor->effective_dimensionality ) {
			
			case 1:
				insert_alpha_sorted( v, *vl_1d, *n_1d, i, names );
				(*n_1d)++;
				break;

			case 2:
				insert_alpha_sorted( v, *vl_2d, *n_2d, i, names );
				(*n_2d)++;
				break;
			
			case 3:
				insert_alpha_sorted( v, *vl_3d, *n_3d, i, names );
				(*n_3d)++;
				break;
			
			case 4:
				insert_alpha_sorted( v, *vl_4d, *n_4d, i, names );
				(*n_4d)++;
				break;

			default:
				insert_alpha_sorted( v, *vl_other, *n_other, i, names );
				(*n_other)++;
				break;
			}

		cursor = cursor->next;
		}
/* printf( "# vars: 1d=%d 2d=%d 3d=%d 4d=%d 5+d=%d\n", *n_1d, *n_2d, *n_3d, *n_4d, *n_other ); */

	free( names );
}

/*************************************************************************************************/
void x_init_widgets_varsel_menu_inner( Widget parent, long *varlist, int nv, char *tag, Widget *var_menu_2use,
			Widget *list_of_sel_widgets, int discard_grpname )
{
	char temp[1024], widget_name[1024], var_menu_name[1024];
	NCVar	*cursor;
	int	i_cursor, i;

	strcpy( temp, tag );
	snprintf( var_menu_name, 1020, "var_menu_%s",  tag     );

	/* For some reason not clear to me, if a widget name has a period in it, the 
	 * X windows system cannot find that widget ???
	 */
	for( i=0; i<strlen(var_menu_name); i++ )
		if( var_menu_name[i] == '.' )
			var_menu_name[i] = '_';

	varsel_menu_widget = XtVaCreateManagedWidget(
		"varsel_menu",
		menuButtonWidgetClass,
		parent,
		XtNmenuName, XtNewString(var_menu_name),
		XtNlabel, temp,
		NULL );

	*var_menu_2use = XtVaCreatePopupShell( 
		XtNewString(var_menu_name),
		simpleMenuWidgetClass,
		varsel_menu_widget,
		NULL );

	for( i=0; i<nv; i++ ) {
		cursor = variables;
		i_cursor = 0;
		while( i_cursor < varlist[i] ) {
			cursor = cursor->next;
			i_cursor++;
			}
		if( discard_grpname )
			unpack_groupname( cursor->name, -2, widget_name );
		else
			snprintf( widget_name, 1020, "%s", cursor->name );
		list_of_sel_widgets[i] = XtVaCreateManagedWidget(
			widget_name,
			smeBSBObjectClass,
			*var_menu_2use,
			NULL );
		XtAddCallback( list_of_sel_widgets[i], XtNcallback, varsel_menu_select, (XtPointer)varlist[i] );
		}
}

/*****************************************************************************************************/
void x_init_widgets_varsel_menu_grp( Widget menu_box, Widget *varsel_menu_widget_list ) 
{
	Stringlist *uniq_groups, *group_cursor;
	int	discard_groupname, nvars, ngrps, *my_grp_num, i, igrp, nv_in_grp;		/* Applies to each var on the global "variables" list; counting starts at zero */
	long	*varlist;
	NCVar	*var_cursor;
	Widget	*var_menu_grp;
	char	widget_name[2048];

	nvars = n_vars_in_list( variables );

	varlist = (long *)malloc( sizeof(long) * nvars );

	/* Sort vars into lists based on their group */
	uniq_groups = get_group_list( variables );
	ngrps = stringlist_len( uniq_groups );

	/* Get group number for each var. First group in list "uniq_groups" is
	 * assigned ID 0, etc.
	 */
	my_grp_num = (int *)malloc( sizeof(int) * nvars );
	var_cursor = variables;
	for( i=0; i<nvars; i++ ) {
		my_grp_num[i] = -1;
		group_cursor = uniq_groups;
		for( igrp=0; igrp<ngrps; igrp++ ) {

			/* Force a var in the root group (in which case it has no slashes) to match
			 * the group named "/"
			 */
			if( (count_nslashes( var_cursor->name )==0) && (strncmp( "/", group_cursor->string, 1)==0)) {
printf( "root check: putting var %s in group %s\n", var_cursor->name, group_cursor->string );
				my_grp_num[i] = igrp;
				break;
				}
			/* Var has at least one slash in its name */
			else if( strncmp( var_cursor->name, group_cursor->string, strlen(group_cursor->string) ) == 0 ) {
/* printf( "non root check: putting var %s in group %s\n", var_cursor->name, group_cursor->string ); */
				/* At this point a var with a group name that STARTS with another group name,
				 * but is longer, will match. So we check to see if the var's name actually
				 * ends with a slash where it should, if this is the right group. If it does
				 * end with a slash, then it's the right group name
				 */
				if( var_cursor->name[ strlen(group_cursor->string) ] == '/' ) {
					my_grp_num[i] = igrp;
					break;
					}
				}

			group_cursor = group_cursor->next;
			}
		if( my_grp_num[i] == -1 ) {
			fprintf( stderr, "x_init_widgets_varsel_menu_grp: Error, did not find group for var named >%s< in the following list of unique groups:\n",
					var_cursor->name );
			stringlist_dump( uniq_groups );
			exit(-1);
			}

		var_cursor = var_cursor->next;
		}

	/* Now go through the groups, and make a menu selection button for
	 * each group that generates a pop-up menu of all vars in that group
	 */
	group_cursor = uniq_groups;
	for( igrp=0; igrp<ngrps; igrp++ ) {
		/* The way the routine x_init_widgets_varsel_menu_inner is set up, it needs
		 * an array of longs that hold the entry number on the global "variables"
		 * list corresponding to each group
		 */
		nv_in_grp = 0;
		for( i=0; i<nvars; i++ ) {
			if( my_grp_num[i] == igrp ) {
				varlist[nv_in_grp] = i;	
				nv_in_grp++;
				}
			}
		var_menu_grp = (Widget *)malloc( sizeof(Widget) );
		discard_groupname = 1;
		snprintf( widget_name, 2040, "%s (%d vars)", group_cursor->string, nv_in_grp );
		if( options.debug ) printf( "x_init_widgets_varsel_menu_grp: making menu for group selection named >%s< with %d vars\n", 
			widget_name, nv_in_grp );
		x_init_widgets_varsel_menu_inner( menu_box, varlist, nv_in_grp, widget_name, var_menu_grp,
			varsel_menu_widget_list, discard_groupname );

		group_cursor = group_cursor->next;
		}

	free( varlist );
}

/*************************************************************************************************
 * The widget selection area can be in one of two different modes.  In "menu" mode, there is one button
 * for all the 1D variables, one button for all the 2D variables, ..., up to one button for all the 
 * variables with dimensionality > 4.  When you click on one of these buttons, a pop-up menu shows
 * all the variables that can be selected that have that number of dims.
 * In "list" mode, all the variables are simply listed in the control panel.
 * For files with a large number of variables, the "list" mode can get too big, hence the
 * existence of the "menu" mode.
 */
void x_init_widgets_varsel_menu( Widget parent )
{
	long 	*vl_1d, *vl_2d, *vl_3d, *vl_4d, *vl_other;	/* These have 0 if the var is NOT in the list, 1 if it is */
	int	discard_groupname, nvars, n_1d, n_2d, n_3d, n_4d, n_other;
	Widget	menu_box;
	char	widget_name[2048];

	menu_box = XtVaCreateManagedWidget(
		"varsel_menu_box",
		boxWidgetClass,
		parent,
		XtNorientation, XtorientHorizontal,
		XtNborderWidth, 0,
		NULL);

	nvars = n_vars_in_list( variables );

	/* Allocate our "global" array that will store each menu selection widget, so we
	 * can later find them to set their sensitivities and otherwise manipulate them.
	 */
	varsel_menu_widget_list = (Widget *)malloc( nvars * sizeof(Widget) );

	/* If we are doing groups, we sort based on their group membership. If we
	 * are not doing groups, we sort based on the variable's number of dims
	 */
	if( options.enable_group_sel ) 
		x_init_widgets_varsel_menu_grp( menu_box, varsel_menu_widget_list );

	else	
		{
		discard_groupname = 0;

		/* Sort vars into lists based on their number of dimensions */
		x_sort_vars_by_ndims( variables, &vl_1d, &n_1d, &vl_2d, &n_2d, &vl_3d, &n_3d, 
				&vl_4d, &n_4d, &vl_other, &n_other );

		if( n_1d > 0 ) {
			snprintf( widget_name, 2040, "(%d) 1d vars", n_1d );
			x_init_widgets_varsel_menu_inner( menu_box, vl_1d, n_1d, widget_name, &var_menu_1d,
				varsel_menu_widget_list, discard_groupname );
			}

		if( n_2d > 0 ) {
			snprintf( widget_name, 2040, "(%d) 2d vars", n_2d );
			x_init_widgets_varsel_menu_inner( menu_box, vl_2d, n_2d, widget_name, &var_menu_2d,
				varsel_menu_widget_list+n_1d, discard_groupname );
			}

		if( n_3d > 0 ) {
			snprintf( widget_name, 2040, "(%d) 3d vars", n_3d );
			x_init_widgets_varsel_menu_inner( menu_box, vl_3d, n_3d, widget_name, &var_menu_3d, 
				varsel_menu_widget_list+n_1d+n_2d, discard_groupname );
			}

		if( n_4d > 0 ) {
			snprintf( widget_name, 2040, "(%d) 4d vars", n_4d );
			x_init_widgets_varsel_menu_inner( menu_box, vl_4d, n_4d, widget_name, &var_menu_4d,
				varsel_menu_widget_list+n_1d+n_2d+n_3d, discard_groupname );
			}

		if( n_other > 0 ) {
			snprintf( widget_name, 2040, "(%d) 5d vars", n_other );
			x_init_widgets_varsel_menu_inner( menu_box, vl_other, n_other, widget_name, &var_menu_other,
				varsel_menu_widget_list+n_1d+n_2d+n_3d+n_4d, discard_groupname );
			}
		}
}

/*************************************************************************************************/
void x_init_widgets_varsel_list( Widget parent )
{
	int	n_vars, n_varsel_boxes, which_box, i, state;
	NCVar	*var;
	char	widget_name[128];
	Widget	w;
	Pixel	col2set;

	/* Arrange the variables in boxes, n_vars_per_row variables to a box */
	n_vars               = n_vars_in_list( variables );
	n_varsel_boxes       = n_vars / app_data.n_vars_per_row + 5;
	var_selection_widget = (Widget *)malloc( n_varsel_boxes*sizeof( Widget ));

	/* Make an array of widgets for the variables; indicate the end of the 
	 * array by a NULL value.
	 */
	varlist_widget = (Widget *)malloc( (n_vars+1)*sizeof(Widget));
	if( varlist_widget == NULL ) {
		fprintf( stderr, "ncview: x_init_widgets: malloc ");
		fprintf( stderr, "failed on varlist_widget initializeation\n" );
		exit( -1 );
		}
	*(varlist_widget+n_vars) = NULL;

	var       = variables;
	which_box = 0;
	for( i=0; i<n_vars; i++ ) {
		if( var == NULL ) 
			{
			fprintf( stderr, "ncview: x_init_widgets: internal ");
			fprintf( stderr, "inconsistency -- empty variable list\n" );
			exit( -1 );
			}
		if( i == 0 ) {
			/* The very first button box! */
			snprintf( widget_name, 127, "varselbox_%1d", which_box+1 );
			*(var_selection_widget+which_box) = XtVaCreateManagedWidget(
				widget_name,
				boxWidgetClass,
				parent,
				XtNorientation, XtorientHorizontal,
				XtNborderWidth, 0,
				NULL);
			snprintf( widget_name, 127, "varlist_label_%1d", which_box+1 );
			varlist_label_widget = XtVaCreateManagedWidget(
				widget_name,
				labelWidgetClass,
				*(var_selection_widget+which_box),
				XtNwidth, app_data.button_width,
				XtNlabel, "Var:",
				XtNborderWidth, 0,
				NULL );
			which_box++;
			}
		else if( (i % app_data.n_vars_per_row) == 0 ) {
			/* A new button box! */
			snprintf( widget_name, 127, "box_%1d", which_box+1 );
			*(var_selection_widget+which_box) = XtVaCreateManagedWidget(
				widget_name,
				boxWidgetClass,
				parent,
				XtNorientation, XtorientHorizontal,
				XtNborderWidth, 0,
				XtNhorizDistance, -app_data.varname_width + app_data.button_width,
				XtNfromVert, *(var_selection_widget + which_box - 1),
				NULL);
			snprintf( widget_name, 127, "varlist_label_%1d", which_box+1 );
			varlist_label_widget = XtVaCreateManagedWidget(
				widget_name,
				labelWidgetClass,
				*(var_selection_widget+which_box),
				XtNwidth, app_data.varname_width,
				XtNborderWidth, 0,
				XtNlabel, "",
				NULL );
			which_box++;
			}

		snprintf( widget_name, 127, "varsel_%s", var->name );
		state = False;
		if( i == 0 )  /* first variable button */
			*(varlist_widget+i) = XtVaCreateManagedWidget(
				widget_name,
				toggleWidgetClass,
				*(var_selection_widget+which_box-1),
				XtNstate, state,
				XtNlabel, var->name,
				XtNsensitive, True,
				XtNwidth, app_data.varname_width,
				NULL );
		else
			*(varlist_widget+i) = XtVaCreateManagedWidget(
				widget_name,
				toggleWidgetClass,
				*(var_selection_widget+which_box-1),
				XtNradioGroup, *varlist_widget,
				XtNstate, state,
				XtNlabel, var->name,
				XtNsensitive, True,
				XtNwidth, app_data.varname_width,
				NULL );
		if( (n_vars > 1) && app_data.var_colors && options.color_by_ndims ) {
			switch( var->effective_dimensionality ) {
				case 1: 
					col2set = app_data.foreground1d;
					break;
				case 2:
					col2set = app_data.foreground2d;
					break;
				case 3:
					col2set = app_data.foreground3d;
					break;
				case 4:
					col2set = app_data.foreground4d;
					break;
				case 5:
				default:
					col2set = app_data.foreground5d;
					break;
				}
			XtVaSetValues(  *(varlist_widget+i),
					XtNforeground, col2set, NULL );
			}
		var = var->next;
		}

	/* In the degenerate case of only one variable, must make it a radio
	 * group by itself; can't do it above, because it needs to know its
	 * own widget number before it can be done.
	 */
	if( n_vars == 1 )
		XtVaSetValues( *varlist_widget, XtNradioGroup, 
				*varlist_widget, NULL );

	i = 0;
	while( (w = *(varlist_widget + i++)) != NULL )
		XtAddCallback( w, XtNcallback, varlist_mod1, NULL );
}

/*************************************************************************************************/
void x_init_widgets_varsel( Widget parent )
{
	if( options.enable_group_sel && (options.varsel_style != VARSEL_MENU)) {
		fprintf( stderr, "Internal error in x_init_widgets_varsel: if group selection is enambled, var selection must be done via menus\n" );
		exit(-1);
		}

	if( options.varsel_style == VARSEL_LIST )
		x_init_widgets_varsel_list( parent );
	else if( options.varsel_style == VARSEL_MENU ) 
		x_init_widgets_varsel_menu( parent );
	else
		{
		fprintf( stderr, "x_init_widgets_varsel: unknown value for options.varsel_style\n" );
		exit(-1);
		}
}

/*************************************************************************************************/
void x_init_widgets_dimlabels( Widget parent )
{
	labels_row_widget = XtVaCreateManagedWidget(
		"label_row",
		boxWidgetClass,
		commandcanvas_widget,
		XtNfromVert, varsel_form_widget,
		XtNorientation, XtorientHorizontal,
		NULL);

	lr_dim_widget = XtVaCreateManagedWidget(
		"label_dimension",
		labelWidgetClass,
		labels_row_widget,
		XtNlabel, "Dim:",
		XtNjustify, XtJustifyRight,
		XtNwidth, 50,
		NULL);

	lr_name_widget = XtVaCreateManagedWidget(
		"label_name",
		labelWidgetClass,
		labels_row_widget,
		XtNlabel, "Name:",
		XtNwidth, app_data.dimlabel_width,
		NULL);

	lr_min_widget = XtVaCreateManagedWidget(
		"label_min",
		labelWidgetClass,
		labels_row_widget,
		XtNlabel, "Min:",
		XtNwidth, app_data.dimlabel_width,
		NULL);

	lr_cur_widget = XtVaCreateManagedWidget(
		"label_cur",
		labelWidgetClass,
		labels_row_widget,
		XtNlabel, "Current:",
		XtNwidth, app_data.dimlabel_width,
		NULL);

	lr_max_widget = XtVaCreateManagedWidget(
		"label_max",
		labelWidgetClass,
		labels_row_widget,
		XtNlabel, "Max:",
		XtNwidth, app_data.dimlabel_width,
		NULL);

	lr_units_widget = XtVaCreateManagedWidget(
		"label_units",
		labelWidgetClass,
		labels_row_widget,
		XtNlabel, "Units:",
		XtNwidth, app_data.dimlabel_width,
		NULL);
}
	
/*************************************************************************************************/
void x_init_widgets( Widget top )
{
	int	n_dims;
	int	max_dims;
	Stringlist *dim_list, *max_dim_list=NULL;
	NCVar	*var;

	x_init_widgets_ccontourpanel( top );

	if( options.display_type == PseudoColor )
		commandcanvas_widget = XtVaCreateManagedWidget(
			"commandcanvas",
			formWidgetClass,
			top,
			XtNcolormap, current_colormap,
			NULL);
	else
		commandcanvas_widget = XtVaCreateManagedWidget(
			"commandcanvas",
			formWidgetClass,
			top,
			NULL);

	/* The labels that hold name of displayed variable, the
	 * variable's range, current location and value, and
	 * the displayed frame number.
	 */
	x_init_widgets_labels( commandcanvas_widget );

	/* First row of buttons, including "Quit" and the
	 * tape-recorder style movement buttons
	 */
	x_init_widgets_buttonbox( commandcanvas_widget );

	/* Second row of options buttons, including colormap selection,
	 * whether or not to invert, mag, transformation, axes,
	 * range, interpolation, and printing
	 */
	x_init_widgets_optionbox( commandcanvas_widget );

	/* The colorbar widget */
	colorbar_form_widget = XtVaCreateManagedWidget(
		"colormapform",
		formWidgetClass,
		commandcanvas_widget,
		XtNfromVert, optionbox_widget,
		XtNresizable, True,
		NULL);

		colorbar_widget = XtVaCreateManagedWidget(
			"colormap",
			simpleWidgetClass,
			colorbar_form_widget,
			XtNwidth,  24,
			XtNheight, 24,
			XtNborderWidth, 0,
			XtNresizable, True,
			XtNleft, XtChainLeft,
			XtNright, XtChainRight,
			NULL);

		XtAddEventHandler( colorbar_widget, ExposureMask, FALSE,
			(XtEventHandler)expose_colorbar, NULL );

	varsel_form_widget = XtVaCreateManagedWidget(
		"varselectform",
		formWidgetClass,
		commandcanvas_widget,
		XtNfromVert, colorbar_form_widget,
		NULL);

	/* the widgets that allow the user to select the variable to display */
	x_init_widgets_varsel( varsel_form_widget );

	/* Description lables for the following dimension entries */
	x_init_widgets_dimlabels( commandcanvas_widget );

	/* Construct an aggregate list which has the most possible scannable
	 * dimensions for all variables in the file.
	 */
	max_dims = -1;
	var      = variables;
	while( var != NULL ) {
		dim_list = fi_scannable_dims( var->first_file->id, var->name );
		n_dims   = stringlist_len( dim_list );
		if( n_dims > max_dims ) {
			max_dims     = n_dims;
			max_dim_list = dim_list;
			}
		var = var->next;
		}
	x_init_dim_info( max_dim_list );
}	

/*************************************************************************************************/
void x_init_pixmaps( Widget top )
{
        create_pixmap( top, &reverse_pixmap,     BUTTON_REWIND      );
	create_pixmap( top, &backwards_pixmap,   BUTTON_BACKWARDS   );
	create_pixmap( top, &pause_pixmap,       BUTTON_PAUSE       );
	create_pixmap( top, &forward_pixmap,     BUTTON_FORWARD     );
	create_pixmap( top, &fastforward_pixmap, BUTTON_FASTFORWARD );

	open_circle_pixmap = XCreateBitmapFromData( XtDisplay(top),
				RootWindowOfScreen(XtScreen(top)),
				(char *)open_circle_bitmap_bits,
				open_circle_bitmap_width,
				open_circle_bitmap_height );

	closed_circle_pixmap = XCreateBitmapFromData( XtDisplay(top),
				RootWindowOfScreen(XtScreen(top)),
				(char *)closed_circle_bitmap_bits,
				closed_circle_bitmap_width,
				closed_circle_bitmap_height );
}	

/*************************************************************************************************/
void create_pixmap( Widget shell_widget, Pixmap *pixmap, int id )
{
	switch( id ) {
		
		case BUTTON_REWIND:
        		*pixmap = XCreateBitmapFromData( XtDisplay(shell_widget),
				RootWindowOfScreen(XtScreen(shell_widget)),
				(char *)reversebitmap_bits,
				reversebitmap_width,
				reversebitmap_height );
			break;

		case BUTTON_BACKWARDS:
        		*pixmap = XCreateBitmapFromData( XtDisplay(shell_widget),
				RootWindowOfScreen(XtScreen(shell_widget)),
				(char *)backbitmap_bits,
				backbitmap_width,
				backbitmap_height );
			break;

		case BUTTON_PAUSE:
        		*pixmap = XCreateBitmapFromData( XtDisplay(shell_widget),
				RootWindowOfScreen(XtScreen(shell_widget)),
				(char *)pausebitmap_bits,
				pausebitmap_width,
				pausebitmap_height );
			break;

		case BUTTON_FORWARD:
        		*pixmap = XCreateBitmapFromData( XtDisplay(shell_widget),
				RootWindowOfScreen(XtScreen(shell_widget)),
				(char *)forwardbitmap_bits,
				forwardbitmap_width,
				forwardbitmap_height );
			break;

		case BUTTON_FASTFORWARD:
        		*pixmap = XCreateBitmapFromData( XtDisplay(shell_widget),
				RootWindowOfScreen(XtScreen(shell_widget)),
				(char *)ffbitmap_bits,
				ffbitmap_width,
	       			ffbitmap_height );
			break;

		default:
			fprintf( stderr, "err: internal error in ncview,\n" );
			fprintf( stderr, "routine create_pixmap: no recognized value\n" );
			fprintf( stderr, ">%d<\n", id );
			exit( 1 );
			break;
		}
}

/*************************************************************************************************/
void x_draw_2d_field( unsigned char *data, size_t width, size_t height, size_t timestep )
{
	Display	*display;
	Screen	*screen;
	static XImage	*ximage;
	XGCValues values;
	GC	gc;
	static	size_t last_width=0L, last_height=0L;
	static 	unsigned char *tc_data=NULL;

#ifdef HAVE_PNG
	if( options.dump_frames )
		dump_to_png( data, width, height, timestep );
#endif

	display = XtDisplay( ccontour_widget );
	screen  = XtScreen ( ccontour_widget );

	if( options.display_type == TrueColor ) {
		/* If the TrueColor data array does not yet exist, 
		 * or is the wrong size, then allocate it.
		 */
		if( tc_data == NULL ) {
			tc_data=(unsigned char *)malloc( server.bitmap_unit*width*height );
			last_width  = width;
			last_height = height;
			}
		else if( (width!=last_width) || (height!=last_height)) {
			free( tc_data );
			last_width  = width;
			last_height = height;
			tc_data=(unsigned char *)malloc( server.bitmap_unit*width*height );
			}
		/* Convert data to TrueColor representation, with
		 * the proper number of bytes per pixel
		 */
		make_tc_data( data, width, height, current_colormap_list->color_list, tc_data );

		ximage  = XCreateImage(
			display,
			XDefaultVisualOfScreen( screen ),
			XDefaultDepthOfScreen ( screen ),
			ZPixmap,
			0,
			(char *)tc_data, 
			(unsigned int)width, (unsigned int)height,
			32, 0 );
		}
	else /* display_type == PseudoColor */
		{
		ximage  = XCreateImage(
			display,
			XDefaultVisualOfScreen( screen ),
			XDefaultDepthOfScreen ( screen ),
			ZPixmap,
			0,
			(char *)data,
			(unsigned int)width, (unsigned int)height,
			8, 0 );
		}

	gc = XtGetGC( ccontour_widget, (XtGCMask)0, &values );

	if( !valid_display )
		return;

	XPutImage(
		display,
		XtWindow( ccontour_widget ),
		gc,
		ximage,
		0, 0, 0, 0,
		(unsigned int)width, (unsigned int)height );
}

/*************************************************************************************************/
void x_set_speed_proc( Widget scrollbar, XtPointer client_data, XtPointer position )
{
	float	f_pos;

	f_pos = *(float *)position;
	options.frame_delay = f_pos;
}

/*************************************************************************************************/
void x_set_2d_size( size_t width, size_t height )
{
	size_t		new_width, new_height, vp_width, vp_height, sb_width;
	Dimension 	widget_height;
	static size_t	last_vp_width=0L, last_vp_height=0L;
	Position	cur_x, cur_y, new_vp_x, new_vp_y;
	int		max_width, max_height;
	int		set_vp_coords;

	/* options.maxsize is given in PERCENT of the maximum screen size in each direction */
	if( options.maxsize_pct == -1 ) {
		max_width  = options.maxsize_width;
		max_height = options.maxsize_height;
		}
	else
		{
		max_width  = (int)( ((double)options.maxsize_pct)/100.0 * (double)server.width  );
		max_height = (int)( ((double)options.maxsize_pct)/100.0 * (double)server.height );
		}

	horiz_scroll_widget =  XtNameToWidget( ccontour_viewport_widget, "horizontal" );
	vert_scroll_widget  =  XtNameToWidget( ccontour_viewport_widget, "vertical"   );

	if( (horiz_scroll_widget==0) && (vert_scroll_widget==0))
		sb_width = 0;
	else
		sb_width = 15;	/* scrollbar width */

	/* Resize actual ccontour image to new proper size */
	new_width = width;
	new_height = height;
	if( options.want_extra_info ) {
		XtVaGetValues( ccontour_info1_widget, 
					XtNheight, &widget_height, NULL );
		new_height += widget_height+4L;
		XtVaGetValues( ccontour_info2_widget, 
					XtNheight, &widget_height, NULL );
		new_height += widget_height+4L;
		}
	XtResizeWidget( ccontour_widget, (Dimension)new_width, (Dimension)new_height, 1 );

	/* See if that is too big given our constraints */
	vp_width = new_width;
	if( vp_width > max_width )
		vp_width = max_width;
	vp_height = new_height;
	if( vp_height > max_height )
		vp_height = max_height;

	if( (vp_width != last_vp_width) || (vp_height != last_vp_height)) {
		XtResizeWidget( ccontourpanel_widget, 	  (Dimension)(vp_width), (Dimension)(vp_height), 0 );
		XtResizeWidget( ccontour_viewport_widget, (Dimension)(vp_width), (Dimension)(vp_height), 0 );
		XtResizeWidget( ccontour_form_widget, 	  (Dimension)(vp_width), (Dimension)(vp_height), 0 );
		}

	XtVaGetValues( ccontour_widget, 
		XtNx, &cur_x, 
		XtNy, &cur_y,
		NULL );
	cur_x = -cur_x;	/* necessary b/c viewport origin generally below and to right of pixmap, not above and to left */
	cur_y = -cur_y;	

	set_vp_coords = 0;
	new_vp_x = cur_x;
	new_vp_y = cur_y;
	if( cur_y + vp_height > new_height ) {
		new_vp_y = new_height - cur_y;
		set_vp_coords = 1;
		}
	if( cur_x + vp_width > new_width ) {
		new_vp_x = new_width - cur_x;
		set_vp_coords = 1;
		}
	if( set_vp_coords )
		XawViewportSetCoordinates( ccontour_viewport_widget, new_vp_x, new_vp_y );

	if( horiz_scroll_widget != 0 ) 
		XawScrollbarSetThumb( horiz_scroll_widget, (float)cur_x/(float)new_width,
							   (float)vp_width/(float)new_width );
	if( vert_scroll_widget != 0 ) 
		XawScrollbarSetThumb( vert_scroll_widget,  (float)cur_y/(float)new_height,
							   (float)vp_height/(float)new_height );

	last_vp_width  = vp_width;
	last_vp_height = vp_height;
}

/*************************************************************************************************/
void x_set_label( int id, char *string )
{
	switch( id ) {
		case LABEL_1:	/* usually title */
			x_set_lab( label1_widget, string, app_data.label_width );
			break;

		case LABEL_2:	/* usually scan variable name */
			x_set_lab( label2_widget, string, app_data.label_width );
			break;

		case LABEL_3:	/* usually scan place */
			x_set_lab( label3_widget, string, app_data.label_width );
			break;

		case LABEL_4:	/* usually min,max info */
			x_set_lab( label4_widget, string, app_data.label_width );
			break;

		case LABEL_5:	/* usually data value */
			x_set_lab( label5_widget, string, app_data.label_width );
			break;

		case LABEL_6:	/* usually scalar coord info */
			x_set_lab( label6_widget, string, app_data.label_width );
			break;

		case LABEL_COLORMAP_NAME:
			x_set_lab( cmap_button_widget, string,
							app_data.button_width );
			break;

		case LABEL_BLOWUP:
			x_set_lab( blowup_widget, string,
							app_data.button_width );
			break;

		case LABEL_TRANSFORM:
			x_set_lab( transform_widget, string,
							app_data.button_width );
			break;

		case LABEL_CCINFO_1:
			x_set_lab( ccontour_info1_widget, string, 0 );
			break;

		case LABEL_CCINFO_2:
			x_set_lab( ccontour_info2_widget, string, 0 );
			break;

		case LABEL_BLOWUP_TYPE:
			x_set_lab( blowup_type_widget, string, app_data.button_width );
			break;

		default:
			fprintf( stderr, "ncview: x_set_label: internal error,\n" );
			fprintf( stderr, "unknown label id %d\n", id );
			exit( -1 );
		}
}

/*************************************************************************************************/
void x_set_lab( Widget w, char *s, int width )
{
	XtVaSetValues( w, XtNlabel, s,     NULL );
	if( width != 0 )
		XtVaSetValues( w, XtNwidth, width, NULL );
}

/*************************** X interface button callbacks ************************/
static void add_callbacks()
{
        XtAddCallback( quit_button_widget,         XtNcallback, quit_mod1,         NULL);
        XtAddCallback( cmap_button_widget,         XtNcallback, cmap_mod1,         NULL );
        XtAddCallback( restart_button_widget,      XtNcallback, restart_mod1,      NULL );
        XtAddCallback( reverse_button_widget,      XtNcallback, reverse_mod1,      NULL );
        XtAddCallback( backwards_button_widget,    XtNcallback, back_mod1,         NULL );
        XtAddCallback( pause_button_widget,        XtNcallback, pause_mod1,        NULL );
        XtAddCallback( forward_button_widget,      XtNcallback, forward_mod1,      NULL );
        XtAddCallback( fastforward_button_widget,  XtNcallback, ff_mod1,           NULL );
        XtAddCallback( edit_button_widget,         XtNcallback, edit_mod1,         NULL );
        XtAddCallback( info_button_widget,         XtNcallback, info_mod1,         NULL );
        XtAddCallback( invert_button_widget,       XtNcallback, invert_mod1,       NULL );
        XtAddCallback( invert_color_button_widget, XtNcallback, invert_color_mod1, NULL );
        XtAddCallback( blowup_widget,              XtNcallback, blowup_mod1,       NULL );
        XtAddCallback( transform_widget,           XtNcallback, transform_mod1,    NULL );
        XtAddCallback( dimset_widget,              XtNcallback, dimset_mod1,       NULL );
        XtAddCallback( range_widget,               XtNcallback, range_mod1,        NULL );
	XtAddCallback( scrollspeed_widget,         XtNjumpProc, x_set_speed_proc,  NULL );
	XtAddCallback( options_button_widget,      XtNcallback, options_mod1,      NULL );

        XtAddCallback( blowup_type_widget,         XtNcallback, blowup_type_mod1,        NULL );
        XtAddCallback( print_button_widget,        XtNcallback, print_button_callback,        NULL );
}

/*************************************************************************************************/
void options_mod1( Widget widget, XtPointer client_data, XtPointer call_data)
{
	in_button_pressed( BUTTON_OPTIONS, MOD_1 );
}

/*************************************************************************************************/
void range_mod1( Widget widget, XtPointer client_data, XtPointer call_data)
{
	in_button_pressed( BUTTON_RANGE, MOD_1 );
}

/*************************************************************************************************/
void print_button_callback( Widget widget, XtPointer client_data, XtPointer call_data)
{
	in_button_pressed( BUTTON_PRINT, MOD_1 );
}

/*************************************************************************************************/
void blowup_type_mod1( Widget widget, XtPointer client_data, XtPointer call_data)
{
	in_button_pressed( BUTTON_BLOWUP_TYPE, MOD_1 );
}

/*************************************************************************************************/
void range_mod3( Widget w, XButtonEvent *event, String *params, Cardinal *num_params )
{
	in_button_pressed( BUTTON_RANGE, MOD_3 );
}

/*************************************************************************************************/
void dimset_mod1( Widget widget, XtPointer client_data, XtPointer call_data)
{
	in_button_pressed( BUTTON_DIMSET, MOD_1 );
}

/*************************************************************************************************/
void quit_mod1( Widget widget, XtPointer client_data, XtPointer call_data)
{
	in_button_pressed( BUTTON_QUIT, MOD_1 );
}

/*************************************************************************************************/
void cmap_mod1( Widget widget, XtPointer client_data, XtPointer call_data)
{
	in_button_pressed( BUTTON_COLORMAP_SELECT, MOD_1 );
}

/*************************************************************************************************/
void cmap_mod3( Widget w, XButtonEvent *event, String *params, Cardinal *num_params )
{
	in_button_pressed( BUTTON_COLORMAP_SELECT, MOD_3 );
}

/*************************************************************************************************/
void do_plot_xy( Widget w, XButtonEvent *event, String *params, Cardinal *num_params )
{
	plot_XY();
}

/*************************************************************************************************/
void do_set_dataedit_place( Widget w, XButtonEvent *event, String *params, Cardinal *num_params )
{
	set_dataedit_place();
}

/*************************************************************************************************/
void do_quit_app( Widget w, XButtonEvent *event, String *params, Cardinal *num_params )
{
	in_button_pressed( BUTTON_QUIT, MOD_1 );
}

/*************************************************************************************************/
void do_set_min_from_curdata( Widget w, XButtonEvent *event, String *params, Cardinal *num_params )
{
	set_min_from_curdata();
}

/*************************************************************************************************/
void do_set_max_from_curdata( Widget w, XButtonEvent *event, String *params, Cardinal *num_params )
{
	set_max_from_curdata();
}

/*************************************************************************************************/
void diminfo_cur_mod1( Widget widget, XtPointer client_data, XtPointer call_data)
{
	int	i = 0;
	Widget	*w;
	String	label;

	while( (w = diminfo_cur_widget+i) != NULL ) {
		if( *w == widget ) {
			XtVaGetValues( *(diminfo_name_widget+i), 
						XtNlabel, &label, NULL );
			in_change_current( label, MOD_1 );
			return;
			}
		i++;
		}
	fprintf( stderr, "ncview: diminfo_cur_mod1 callback: can't find " );
	fprintf( stderr, "widget for the pressed button\n" );
	exit( -1 );
}

/*************************************************************************************************/
void diminfo_cur_mod2( Widget widget, XButtonEvent *event, String *params, 
							Cardinal *num_params )
{
	int	i = 0;
	Widget	*w;
	String	label;

	while( (w = diminfo_cur_widget+i) != NULL ) {
		if( *w == widget ) {
			XtVaGetValues( *(diminfo_name_widget+i), 
						XtNlabel, &label, NULL );
			in_change_current( label, MOD_2 );
			return;
			}
		i++;
		}
	fprintf( stderr, "ncview: diminfo_cur_mod2 callback: can't find " );
	fprintf( stderr, "widget for the pressed button\n" );
	exit( -1 );
}

/*************************************************************************************************/
void diminfo_cur_mod3( Widget widget, XButtonEvent *event, String *params, 
							Cardinal *num_params )
{
	int	i = 0;
	Widget	*w;
	String	label;

	while( (w = diminfo_cur_widget+i) != NULL ) {
		if( *w == widget ) {
			XtVaGetValues( *(diminfo_name_widget+i), 
						XtNlabel, &label, NULL );
			in_change_current( label, MOD_3 );
			return;
			}
		i++;
		}
	fprintf( stderr, "ncview: diminfo_cur_mod3 callback: can't find " );
	fprintf( stderr, "widget for the pressed button\n" );
	exit( -1 );
}

/*************************************************************************************************/
void diminfo_cur_mod4( Widget widget, XButtonEvent *event, String *params, 
							Cardinal *num_params )
{
	int	i = 0;
	Widget	*w;
	String	label;

	while( (w = diminfo_cur_widget+i) != NULL ) {
		if( *w == widget ) {
			XtVaGetValues( *(diminfo_name_widget+i), 
						XtNlabel, &label, NULL );
			in_change_current( label, MOD_4 );
			return;
			}
		i++;
		}
	fprintf( stderr, "ncview: diminfo_cur_mod4 callback: can't find " );
	fprintf( stderr, "widget for the pressed button\n" );
	exit( -1 );
}

/*************************************************************************************************/
void restart_mod1( Widget widget, XtPointer client_data, XtPointer call_data)
{
	in_button_pressed( BUTTON_RESTART, MOD_1 );
}

/*************************************************************************************************/
void reverse_mod1( Widget widget, XtPointer client_data, XtPointer call_data)
{
	in_button_pressed( BUTTON_REWIND, MOD_1 );
}

/*************************************************************************************************/
void reverse_mod2( Widget w, XButtonEvent *event, String *params, Cardinal *num_params )
{
	in_button_pressed( BUTTON_REWIND, MOD_2 );
}

/*************************************************************************************************/
void back_mod2( Widget w, XButtonEvent *event, String *params, Cardinal *num_params )
{
	in_button_pressed( BUTTON_BACKWARDS, MOD_2 );
}

/*************************************************************************************************/
void back_mod1( Widget widget, XtPointer client_data, XtPointer call_data)
{
	in_button_pressed( BUTTON_BACKWARDS, MOD_1 );
}

/*************************************************************************************************/
void pause_mod1( Widget widget, XtPointer client_data, XtPointer call_data)
{
	in_button_pressed( BUTTON_PAUSE, MOD_1 );
}

/*************************************************************************************************/
void forward_mod1( Widget widget, XtPointer client_data, XtPointer call_data)
{
	in_button_pressed( BUTTON_FORWARD, MOD_1 );
}

/*************************************************************************************************/
void forward_mod2( Widget w, XButtonEvent *event, String *params, Cardinal *num_params )
{
	in_button_pressed( BUTTON_FORWARD, MOD_2 );
}

/*************************************************************************************************/
void edit_mod1( Widget widget, XtPointer client_data, XtPointer call_data)
{
	in_button_pressed( BUTTON_EDIT, MOD_1 );
}

/*************************************************************************************************/
void info_mod1( Widget widget, XtPointer client_data, XtPointer call_data)
{
	in_button_pressed( BUTTON_INFO, MOD_1 );
}

/*************************************************************************************************/
void ff_mod1( Widget widget, XtPointer client_data, XtPointer call_data)
{
	in_button_pressed( BUTTON_FASTFORWARD, MOD_1 );
}

/*************************************************************************************************/
void fastforward_mod2( Widget w, XButtonEvent *event, String *params, Cardinal *num_params )
{
	in_button_pressed( BUTTON_FASTFORWARD, MOD_2 );
}

/*************************************************************************************************/
void invert_mod1( Widget widget, XtPointer client_data, XtPointer call_data)
{
	in_button_pressed( BUTTON_INVERT_PHYSICAL, MOD_1 );
}

/*************************************************************************************************/
void invert_color_mod1( Widget widget, XtPointer client_data, XtPointer call_data)
{
	in_button_pressed( BUTTON_INVERT_COLORMAP, MOD_1 );
}

/*************************************************************************************************/
void blowup_mod1( Widget widget, XtPointer client_data, XtPointer call_data)
{
	in_button_pressed( BUTTON_BLOWUP, MOD_1 );
}

/*************************************************************************************************/
void blowup_mod4( Widget w, XButtonEvent *event, String *params, Cardinal *num_params )
{
	in_button_pressed( BUTTON_BLOWUP, MOD_4 );
}

/*************************************************************************************************/
void blowup_mod3( Widget w, XButtonEvent *event, String *params, Cardinal *num_params )
{
	in_button_pressed( BUTTON_BLOWUP, MOD_3 );
}

/*************************************************************************************************/
void blowup_mod2( Widget w, XButtonEvent *event, String *params, Cardinal *num_params )
{
	in_button_pressed( BUTTON_BLOWUP, MOD_2 );
}

/*************************************************************************************************/
void transform_mod1( Widget widget, XtPointer client_data, XtPointer call_data)
{
	in_button_pressed( BUTTON_TRANSFORM, MOD_1 );
}

/*======================================================================================
 * Note that this callback is used ONLY when the 'list' type variable selection is
 * being used.  When the 'menu' type is being used, a different callback is invoked
 * from those variable selection widgets (varsel_menu_select).
 */
void varlist_mod1( Widget widget, XtPointer client_data, XtPointer call_data)
{
	char	*widget_name;

	/* Only respond to 'set' calls, not 'unset' calls... */
	if( call_data == 0 )
		return;

	widget_name = XawToggleGetCurrent( *varlist_widget );
	if( widget_name == NULL )	/* happens because first call to */
		return;			/* ToggleSet unsets the previous */
					/* selection, which is NULL      */
	/* advance past the leading 'varsel_' */
	widget_name += strlen( "varsel_" );
	in_variable_selected( widget_name );
}

/*************************************************************************************************/
void x_set_var_sensitivity( char *varname, int sens )
{
	int	i = 0;
	String	label;

	if( options.varsel_style == VARSEL_LIST ) {
		while( (varlist_widget+i) != NULL ) {
			XtVaGetValues( *(varlist_widget+i), XtNlabel, &label, NULL );
			if( strcmp( varname, label ) == 0 ) {
				XtVaSetValues(  *(varlist_widget+i),
						XtNsensitive, sens, NULL );
				if( sens == FALSE ) 
					XtVaSetValues(  *(varlist_widget+i),
						XtNstate, FALSE, NULL );
				return;
				}
			i++;
			}
		}
		
	else if( options.varsel_style == VARSEL_MENU ) {
		for( i=0; i<n_vars_in_list(variables); i++ ) {
			XtVaGetValues( *(varsel_menu_widget_list+i), XtNlabel, &label, NULL );
			if( strcmp( varname, label ) == 0 ) {
				XtVaSetValues(  *(varsel_menu_widget_list+i),
						XtNsensitive, sens, NULL );
				if( sens == FALSE ) 
					XtVaSetValues(  *(varsel_menu_widget_list+i),
						XtNstate, FALSE, NULL );
				return;
				}
			}
		}
		
	else
		{
		fprintf( stderr, "x_set_var_sensitivity: unknown value for options.varsel_style\n" );
		exit(-1);
		}

	fprintf( stderr, "ncview: x_set_var_sensitivity: can't find " );
	fprintf( stderr, "widget for variable %s\n", varname );
	exit( -1 );
}

/*************************************************************************************************/
void x_process_user_input( void )
{
	for(;;)
		{
		XtAppNextEvent( x_app_context, &event );
		XtDispatchEvent( &event );
		}
}
	
/*************************************************************************************************/
void x_timer_set( XtTimerCallbackProc procedure, XtPointer client_arg, unsigned long delay_millisec )
{
	timer = XtAppAddTimeOut( 
		x_app_context,
		delay_millisec,
		procedure,
		client_arg );
	timer_enabled = TRUE;
}

/*************************************************************************************************/
void x_timer_clear( void )
{
	if( timer_enabled ) {
		XtRemoveTimeOut( timer );
		timer_enabled = FALSE;
		}
}

/*************************************************************************************************/
void x_indicate_active_var( char *var_name )
{
	Widget	*w;
	String	label;
	int	i = 0;

	w = varlist_widget;
	if( options.varsel_style == VARSEL_LIST ) {
		while( *w != NULL ) {
			XtVaGetValues( *w, XtNlabel, &label, NULL );
			if( strcmp( label, var_name ) == 0 ) {
				XtVaGetValues( *(varlist_widget+i), XtNradioData, &label, NULL );
				XawToggleSetCurrent( *varlist_widget, (XtPointer)label );
				return;
				}
			w = (varlist_widget + ++i );
			}
		}
	else if( options.varsel_style == VARSEL_MENU ) {
		/* When you have a menu, there is no corresponding feature to a
		 * highlighted single toggle widget (part of a radio group).  Menu
		 * widgets don't support toggling, nor radio group behavior.  However,
		 * setting the toggle state on the list selection widget invokes
		 * its callback, so we have to simulate that behavior as well.
		 */
		in_variable_selected( var_name );
		return;
		}
	else
		{
		fprintf( stderr, "x_indicate_active_var: error: unknown value of options.varsel_style\n" );
		exit(-1);
		}

	fprintf( stderr, "ncview: x_indicate_active_var: cannot find " );
	fprintf( stderr, "widget for variable named >%s<\n", var_name );
	exit( -1 );
}

/*************************************************************************************************/
void x_indicate_active_dim( int dimension, char *dim_name )
{
	Widget	*w;
	String	label;
	int	i = 0;
	char	new_label[ 132 ];
	char	dim_name_ng[ MAX_NC_NAME ];

	varname_no_groups( dim_name, dim_name_ng, NULL );

	if( dimension == DIMENSION_X )
		snprintf( new_label, 130, "X:" );
	else if( dimension == DIMENSION_Y )
		snprintf( new_label, 130, "Y:" );
	else if( dimension == DIMENSION_SCAN )
		snprintf( new_label, 130, "Scan:" );
	else if( dimension == DIMENSION_NONE )
		snprintf( new_label, 130, " " );
	else
		{
		fprintf( stderr, "ncview: x_indicate_active_dim: unknown " );
		fprintf( stderr, "dimension received: %d\n", dimension     );
		exit( -1 );
		}

	i = 0;
	w = diminfo_name_widget;
	while( *w != NULL ) {
		XtVaGetValues( *w, XtNlabel, &label, NULL );
		if( strcmp( label, dim_name_ng ) == 0 ) {
			XtVaSetValues( *(diminfo_dim_widget+i), 
					XtNlabel, new_label,
					XtNwidth, app_data.dimlabel_width, NULL);
			return;
			}
		w = (diminfo_name_widget + ++i );
		}

	fprintf( stderr, "ncview: x_indicate_active_dim: cannot find " );
	fprintf( stderr, "widget for dimension %d, named %s\n", 
							dimension, dim_name_ng );
	exit( -1 );
}

/*************************************************************************************************/
void x_set_sensitive( int button_id, int state )
{
	switch( button_id ) {
		case BUTTON_BLOWUP_TYPE:	
			XtVaSetValues( blowup_type_widget, XtNsensitive, state, NULL );
			break;

		case BUTTON_RANGE:	
			XtVaSetValues( range_widget, XtNsensitive, state, NULL );
			break;

		case BUTTON_DIMSET:	
			XtVaSetValues( dimset_widget, XtNsensitive, state, NULL );
			break;

		case BUTTON_RESTART:
			XtVaSetValues( restart_button_widget, XtNsensitive, state, NULL );
			break;

		case BUTTON_REWIND:	
			XtVaSetValues( reverse_button_widget, XtNsensitive, state, NULL );
			break;

		case BUTTON_BACKWARDS:
			XtVaSetValues( backwards_button_widget, XtNsensitive, state, NULL );
			break;

		case BUTTON_PAUSE:
			XtVaSetValues( pause_button_widget, XtNsensitive, state, NULL );
			break;

		case BUTTON_FORWARD:
			XtVaSetValues( forward_button_widget, XtNsensitive, state, NULL );
			break;

		case BUTTON_FASTFORWARD:
			XtVaSetValues( fastforward_button_widget, XtNsensitive, state, NULL );
			break;

		case BUTTON_COLORMAP_SELECT:
			XtVaSetValues( cmap_button_widget, XtNsensitive, state, NULL );
			break;

		case BUTTON_INVERT_PHYSICAL:
			XtVaSetValues( invert_button_widget, XtNsensitive, state, NULL );
			break;

		case BUTTON_INVERT_COLORMAP:
			XtVaSetValues( invert_color_button_widget, XtNsensitive, state, NULL );
			break;

		case BUTTON_BLOWUP:
			XtVaSetValues( blowup_widget, XtNsensitive, state, NULL );
			break;

		case BUTTON_TRANSFORM:
			XtVaSetValues( transform_widget, XtNsensitive, state, NULL );
			break;

		case BUTTON_EDIT:
			XtVaSetValues( edit_button_widget, XtNsensitive, state, NULL );
			break;

		case BUTTON_INFO:
			XtVaSetValues( info_button_widget, XtNsensitive, state, NULL );
			break;

		case BUTTON_PRINT:
			XtVaSetValues( print_button_widget, XtNsensitive, state, NULL );
			break;
		}
}

/*************************************************************************************************/
void x_force_set_invert_state( int state )
{
	XtVaSetValues( invert_button_widget, XtNstate, state, NULL );
}

/*************************************************************************************************/
void x_clear_dim_buttons()
{
	Widget	w;
	int	i;

	if( xdimlist_widget == NULL )
		return;

	i=0;
	while( (w = *(xdimlist_widget + i)) != NULL )
		i++;
	while( i>0 ) {
		i--;
		XtDestroyWidget( *(xdimlist_widget + i ));
		}

	i=0;
	while( (w = *(ydimlist_widget + i)) != NULL )
		i++;
	while( i>0 ) {
		i--;
		XtDestroyWidget( *(ydimlist_widget + i ));
		}
}

/*************************************************************************************************/
int x_set_scan_dims( Stringlist *dim_list, char *x_axis_name, char *y_axis_name,
	Stringlist **new_dim_list )
{
	Position	width, height, root_x, root_y;
	char		*new_xdim_name, *new_ydim_name;
	char		*x_prefix = "xsel_", *y_prefix = "ysel_"; 
	int		i;

	if( options.display_type == TrueColor )
		dimsel_popup_widget = XtVaCreatePopupShell(
			"Dimension Select",
			transientShellWidgetClass,
			topLevel,
			NULL );
	else if( valid_display ) 
		dimsel_popup_widget = XtVaCreatePopupShell(
			"Dimension Select",
			transientShellWidgetClass,
			topLevel,
			XtNcolormap, current_colormap,
			NULL );
	else
		dimsel_popup_widget = XtVaCreatePopupShell(
			"Dimension Select",
			transientShellWidgetClass,
			topLevel,
			NULL );

	dimsel_popupcanvas_widget = XtVaCreateManagedWidget(
		"dimsel_popupcanvas",
		formWidgetClass,
		dimsel_popup_widget,
		XtNborderWidth, 0,
		NULL);

	ydim_selection_widget = XtVaCreateManagedWidget(
		"ydimselectbox",
		boxWidgetClass,
		dimsel_popupcanvas_widget,
		XtNorientation, XtorientHorizontal,
		NULL);

	ydimlist_label_widget = XtVaCreateManagedWidget(
		"ydimlist_label",
		labelWidgetClass,
		ydim_selection_widget,
		XtNwidth, app_data.button_width,
		XtNjustify, XtJustifyRight,
		XtNlabel, "Y Dim:",
		NULL );

	xdim_selection_widget = XtVaCreateManagedWidget(
		"xdimselectbox",
		boxWidgetClass, 
		dimsel_popupcanvas_widget,
		XtNfromVert, ydim_selection_widget,
		XtNorientation, XtorientHorizontal,
		NULL);

	xdimlist_label_widget = XtVaCreateManagedWidget(
		"xdimlist_label",
		labelWidgetClass,
		xdim_selection_widget,
		XtNwidth, app_data.button_width,
		XtNjustify, XtJustifyRight,
		XtNlabel, "X Dim:",
		NULL );

	x_make_dim_button_util( DIMENSION_Y,    dim_list, y_prefix,
			&ydimlist_widget, ydim_selection_widget, y_axis_name );
	x_make_dim_button_util( DIMENSION_X,    dim_list, x_prefix,    
			&xdimlist_widget, xdim_selection_widget, x_axis_name );

	dimsel_ok_button_widget = XtVaCreateManagedWidget(
		"OK",
		commandWidgetClass,
		dimsel_popupcanvas_widget,
		XtNfromVert, xdim_selection_widget,
		NULL);

	dimsel_cancel_button_widget = XtVaCreateManagedWidget(
		"Cancel",
		commandWidgetClass,
		dimsel_popupcanvas_widget,
		XtNfromVert, xdim_selection_widget,
		XtNfromHoriz, dimsel_ok_button_widget,
		NULL);

        XtAddCallback( dimsel_ok_button_widget, XtNcallback, 
			dimsel_callback, (XtPointer)MESSAGE_OK );
        XtAddCallback( dimsel_cancel_button_widget, XtNcallback, 
			dimsel_callback, (XtPointer)MESSAGE_CANCEL );

	XtVaGetValues    ( commandcanvas_widget, XtNwidth,  &width, 
					XtNheight, &height, NULL );
	XtTranslateCoords( commandcanvas_widget, (Position)width,
					(Position)height, &root_x, &root_y );
	XtVaSetValues    ( dimsel_popup_widget, XtNx, root_x/2, XtNy, root_y/2, NULL );
	XtPopup          ( dimsel_popup_widget, XtGrabExclusive );

	if( options.display_type == PseudoColor )
		XSetWindowColormap( XtDisplay(topLevel), XtWindow(dimsel_popup_widget), 
				current_colormap );

	/* This mini main event loop just handles the dimension selection popup */
	dimsel_popup_done = FALSE;
	while( ! dimsel_popup_done ) {
		XtAppNextEvent( x_app_context, &event );
		XtDispatchEvent( &event );
		}

	if( dimsel_popup_result == MESSAGE_OK ) {
		new_ydim_name = XawToggleGetCurrent( *ydimlist_widget );
		new_xdim_name = XawToggleGetCurrent( *xdimlist_widget );

		/* advance past the leading prefixes */
		for( i=strlen(y_prefix); *(new_ydim_name+i) != '\0'; i++ )
			*(new_ydim_name+i-strlen(y_prefix)) = *(new_ydim_name+i);
		*(new_ydim_name+i-strlen(y_prefix)) = '\0';
		for( i=strlen(x_prefix); *(new_xdim_name+i) != '\0'; i++ )
			*(new_xdim_name+i-strlen(x_prefix)) = *(new_xdim_name+i);
		*(new_xdim_name+i-strlen(x_prefix)) = '\0';
		stringlist_add_string( new_dim_list, new_ydim_name, NULL, SLTYPE_NULL );
		stringlist_add_string( new_dim_list, new_xdim_name, NULL, SLTYPE_NULL );
		}

	XtPopdown( dimsel_popup_widget );

	XtDestroyWidget( dimsel_cancel_button_widget );
	XtDestroyWidget( dimsel_ok_button_widget   );
	XtDestroyWidget( dimsel_popupcanvas_widget );
	XtDestroyWidget( dimsel_popup_widget       );

	dimsel_popup_widget = NULL;

	return( dimsel_popup_result );
}

/*************************************************************************************************/
void dimsel_callback( Widget widget, XtPointer client_data, XtPointer call_data)
{
	long	l_client_data;

	l_client_data = (long)client_data;

	dimsel_popup_result = (int)l_client_data;
	dimsel_popup_done   = TRUE;
}

/*************************************************************************************************/
void x_make_dim_button_util( int dimension, Stringlist *dim_list, char *prefix, 
	Widget **dimlist_widget, Widget parent_widget, char *selected_name )
{
	Widget		*w;
	int	   	i, n_dims;
	Stringlist 	*s;
	char		widget_name[ 64 ];
	String		label;

	/* Make the widget array; set the last element to NULL so that 
	 * we can find it later.
	 */
	n_dims          = stringlist_len( dim_list );
	*dimlist_widget = (Widget *)malloc( (n_dims+1)*sizeof(Widget));
	if( *dimlist_widget == NULL ) {
		fprintf( stderr, "ncview: x_make_dim_buttons: malloc ");
		fprintf( stderr, "failed on dimlist_widget initialization\n" );
		exit( -1 );
		}
	*(*dimlist_widget+n_dims) = NULL;

	s = dim_list;
	i = 0;
	while( s != NULL )
		{
		snprintf( widget_name, 62, "%s%s", prefix, s->string );
		if( i == 0 )
			*(*dimlist_widget + i) = XtVaCreateManagedWidget(
				widget_name,
				toggleWidgetClass,
				parent_widget,
				XtNwidth, app_data.button_width,
				XtNlabel, s->string,
				NULL);
		else
			*(*dimlist_widget + i) = XtVaCreateManagedWidget(
				widget_name,
				toggleWidgetClass,
				parent_widget,
				XtNlabel, s->string,
				XtNwidth, app_data.button_width,
				XtNradioGroup, **dimlist_widget,
				NULL);
		s = s->next;
		i++;
		}

	/* Now, set the initial value on the appropriate widget */
	i = 0;
	w = *dimlist_widget;
	while( *w != NULL ) {
		XtVaGetValues( *w, XtNlabel, &label, NULL );
		if( strcmp( label, selected_name ) == 0 ) {
			XtVaGetValues( *(*dimlist_widget+i), XtNradioData, &label, NULL );
			XawToggleSetCurrent( **dimlist_widget, (XtPointer)label );
			return;
			}
		w = (*dimlist_widget + ++i );
		}
	fprintf( stderr, "?didn't find a match for name >%s<\n",
				selected_name );
}

/*************************************************************************************************/
void check_app_res( AppDataPtr ad )
{
	if( ad->delta_step > 100 ) {
		fprintf( stderr, "ncview: check_app_data: error in resource " );
		fprintf( stderr, "file entry for deltaStep.  Syntax is:\n" );
		fprintf( stderr, "if deltaStep > 0, then it indicates the " );
		fprintf( stderr, "integer *percent* (from 1 to 100) to step \n");
		fprintf( stderr, "on each press of the forward or backward ");
		fprintf( stderr, "buttons while holding down the Ctrl key.\n");
		fprintf( stderr, "If deltaStep < 0, it indicates the number ");
		fprintf( stderr, "of frames to step in such events.\n" );
		exit( -1 );
		}
	
	if( (ad->button_width > 500) ||
	    (ad->button_width < 10)   ) {
		fprintf( stderr, "ncview: check_app_data: error in resource " );
		fprintf( stderr, "file entry for buttonWidth.  Acceptable\n" );
		fprintf( stderr, "range is 10 to 500\n" );
		exit( -1 );
		}

	if( (ad->varname_width > 1024) ||
	    (ad->varname_width < 10)   ) {
		fprintf( stderr, "ncview: check_app_data: error in resource " );
		fprintf( stderr, "file entry for varnameWidth.  Acceptable\n" );
		fprintf( stderr, "range is 10 to 1024\n" );
		exit( -1 );
		}

	if( (ad->label_width > 2000) ||
	    (ad->label_width < 100)   ) {
		fprintf( stderr, "ncview: check_app_data: error in resource " );
		fprintf( stderr, "file entry for labelWidth.  Acceptable\n" );
		fprintf( stderr, "range is 100 to 2000\n" );
		exit( -1 );
		}

	if( (ad->n_vars_per_row > 20) || 
	    (ad->n_vars_per_row < 1 ) ) {
		fprintf( stderr, "ncview: check_app_data: error in resource " );
		fprintf( stderr, "file entry for NVarsPerRow.  Acceptable\n" );
		fprintf( stderr, "range is 1 to 20\n" );
		exit( -1 );
		}

	if( fabs(ad->version) < 0.001 ) {
		fprintf(stderr, "Note: no Ncview app-defaults file found, using internal defaults\n" );
		}
	else if( fabs(ad->version - APP_RES_VERSION) > 0.001 ) {
		fprintf(stderr, "Note: incorrect version of Ncview app-defaults file found (was expecting %.2f, found %.2f), using internal defaults\n", APP_RES_VERSION, ad->version);
		}

}

/*************************************************************************************************/
void x_error( char *message )
{
	x_dialog( message, NULL, FALSE );
}

/*************************************************************************************************/
int x_dialog( char *message, char *ret_string, int want_cancel_button )
{
	Position	width, height, root_x, root_y;
	static Widget	ok_button_widget, cancel_button_widget;

	if( options.display_type == TrueColor )
		error_popup_widget = XtVaCreatePopupShell(
			"popup",
			transientShellWidgetClass,
			topLevel,
			NULL );
	else if( valid_display ) 
		error_popup_widget = XtVaCreatePopupShell(
			"popup",
			transientShellWidgetClass,
			topLevel,
			XtNcolormap, current_colormap,
			NULL );
	else
		error_popup_widget = XtVaCreatePopupShell(
			"popup",
			transientShellWidgetClass,
			topLevel,
			NULL );

	error_popupcanvas_widget = XtVaCreateManagedWidget(
		"error_popupcanvas",
		formWidgetClass,
		error_popup_widget,
		XtNborderWidth, 0,
		NULL);

	if( ret_string != NULL )
		error_popupdialog_widget = XtVaCreateManagedWidget(
			"error_popupdialog",	
			dialogWidgetClass,	
			error_popupcanvas_widget,
			XtNlabel, message,
			XtNborderWidth, 0,
			XtNvalue, ret_string,
			NULL );
	else
		error_popupdialog_widget = XtVaCreateManagedWidget(
			"error_popupdialog",
			dialogWidgetClass,
			error_popupcanvas_widget,
			XtNborderWidth, 0,
			XtNlabel, message,
			NULL );

	ok_button_widget = XtVaCreateManagedWidget(
		"OK",
		commandWidgetClass,
		error_popupdialog_widget,
		NULL);

        XtAddCallback( ok_button_widget,     XtNcallback, 
				error_popup_callback, (XtPointer)MESSAGE_OK);

	if( want_cancel_button ) {
		cancel_button_widget = XtVaCreateManagedWidget(
			"Cancel",
			commandWidgetClass,
			error_popupdialog_widget,
			XtNfromHoriz, ok_button_widget,
			NULL);
	        XtAddCallback( cancel_button_widget, XtNcallback,
				error_popup_callback, (XtPointer)MESSAGE_CANCEL);
		}

	/* Move the dialog to a reasonable location and pop it up */
	XtVaGetValues    ( commandcanvas_widget, XtNwidth,  &width, 
					XtNheight, &height, NULL );
	XtTranslateCoords( commandcanvas_widget, (Position)width,
					(Position)height, &root_x, &root_y );
	XtVaSetValues    ( error_popup_widget, XtNx, root_x/2, XtNy, root_y/2, NULL );
	XtPopup          ( error_popup_widget, XtGrabExclusive );

	if( options.display_type == PseudoColor )
		XSetWindowColormap( XtDisplay(topLevel), XtWindow(error_popup_widget), 
				current_colormap );

	/*
	XtInstallAccelerators( error_popupdialog_widget, ok_button_widget );
	*/
	while( ! error_popup_done ) {
		XtAppNextEvent( x_app_context, &event );
		XtDispatchEvent( &event );
		}
	error_popup_done = FALSE;

	if( ret_string != NULL )
		strcpy( ret_string, XawDialogGetValueString( error_popupdialog_widget ));
	XtPopdown( error_popup_widget );

	if( want_cancel_button )
		XtDestroyWidget( cancel_button_widget );
	XtDestroyWidget( ok_button_widget         );
	XtDestroyWidget( error_popupdialog_widget );
	XtDestroyWidget( error_popupcanvas_widget );
	XtDestroyWidget( error_popup_widget       );
	error_popup_widget = NULL;

	return( error_popup_result );
}

/*************************************************************************************************/
void error_popup_callback( Widget widget, XtPointer client_data, XtPointer call_data)
{
	long	l_client_data;

	l_client_data = (long)client_data;

	error_popup_result = (int)l_client_data;
	error_popup_done   = TRUE;
}

/*************************************************************************************************/
void x_fill_dim_info( NCDim *d, int please_flip )
{
	Widget	*w;
	String	widget_name;
	int	i;
	char	temp_label[132];
	char 	dimname_ng[ MAX_NC_NAME ];

	/* Get the dimname sans group */
	varname_no_groups( d->name, dimname_ng, NULL );

	/* first, find the row we want */
	i = 0;
	while( (w = diminfo_name_widget + i) != NULL ) {
		XtVaGetValues( *w, XtNlabel, &widget_name, NULL );
		if( strcmp( widget_name, dimname_ng ) == 0 ) {

			if( please_flip )
				snprintf( temp_label, 130, "%g", d->max );
			else
				snprintf( temp_label, 130, "%g", d->min );
			XtVaSetValues( *(diminfo_min_widget+i),
				XtNlabel, temp_label,
				XtNwidth, app_data.dimlabel_width,
				NULL );

			if( please_flip )
				snprintf( temp_label, 130, "%g", d->min );
			else
				snprintf( temp_label, 130, "%g", d->max );
			XtVaSetValues( *(diminfo_max_widget+i),
				XtNlabel, temp_label,
				XtNwidth, app_data.dimlabel_width,
				NULL );

			if( d->units == NULL )
				XtVaSetValues( *(diminfo_units_widget+i),
					XtNlabel, "-",
					XtNwidth, app_data.dimlabel_width,
					NULL );
			else
				XtVaSetValues( *(diminfo_units_widget+i),
					XtNlabel, limit_string(d->units),
					XtNwidth, app_data.dimlabel_width,
					NULL );

			return;
			}
		i++;
		}

	fprintf( stderr, "ncview: x_fill_dim_info: error, can't find " );
	fprintf( stderr, "dim info widget named \"%s\"\n", dimname_ng );
	exit( -1 );
}

/*************************************************************************************************/
void x_set_cur_dim_value( char *dim_name, char *string )
{
	int	i;
	Widget	*w;
	String	label;
	char	dim_name_ng[ MAX_NC_NAME ];

	varname_no_groups( dim_name, dim_name_ng, NULL );

	i = 0;
	while( (w = diminfo_name_widget+i) != NULL ) {
		XtVaGetValues( *w, XtNlabel, &label, NULL );
		if( strcmp( label, dim_name_ng ) == 0 ) {
			XtVaSetValues( *(diminfo_cur_widget+i), 
				XtNlabel, string, NULL );
			return;
			}
		i++;
		}
	fprintf( stderr, "ncview: x_set_cur_dim: error; widget for dimension ");
	fprintf( stderr, "named \"%s\" not found.\n", dim_name_ng );
	exit( -1 );
}

/*************************************************************************************************/
void x_init_dim_info( Stringlist *dims )
{
	long	ll;
	int	n_dims;
	char	widget_name[128];
	Dimension bb_width, bb_height;

	XtVaGetValues( labels_row_widget, XtNwidth,  &bb_width,
					  XtNheight, &bb_height, NULL );

	x_clear_dim_info();
	n_dims = stringlist_len( dims );
	diminfo_row_widget = (Widget *)malloc( (n_dims+1)*sizeof(Widget));
	if( diminfo_row_widget == NULL ) {
		fprintf( stderr, "ncview: x_init_dim_info: malloc failed ");
		fprintf( stderr, "initializing %d diminfo_row widgets",
							n_dims );
		exit( -1 );
		}
	diminfo_dim_widget = (Widget *)malloc( (n_dims+1)*sizeof(Widget));
	if( diminfo_dim_widget == NULL ) {
		fprintf( stderr, "ncview: x_init_dim_info: malloc failed ");
		fprintf( stderr, "initializing %d diminfo_dim widgets",
							n_dims );
		exit( -1 );
		}
	diminfo_name_widget = (Widget *)malloc( (n_dims+1)*sizeof(Widget));
	if( diminfo_name_widget == NULL ) {
		fprintf( stderr, "ncview: x_init_dim_info: malloc failed ");
		fprintf( stderr, "initializing %d diminfo_name widgets",
							n_dims );
		exit( -1 );
		}
	diminfo_min_widget = (Widget *)malloc( (n_dims+1)*sizeof(Widget));
	if( diminfo_min_widget == NULL ) {
		fprintf( stderr, "ncview: x_init_dim_info: malloc failed ");
		fprintf( stderr, "initializing %d diminfo_min widgets",
							n_dims );
		exit( -1 );
		}
	diminfo_cur_widget = (Widget *)malloc( (n_dims+1)*sizeof(Widget));
	if( diminfo_cur_widget == NULL ) {
		fprintf( stderr, "ncview: x_init_dim_info: malloc failed ");
		fprintf( stderr, "initializing %d diminfo_cur widgets",
							n_dims );
		exit( -1 );
		}
	diminfo_max_widget = (Widget *)malloc( (n_dims+1)*sizeof(Widget));
	if( diminfo_max_widget == NULL ) {
		fprintf( stderr, "ncview: x_init_dim_info: malloc failed ");
		fprintf( stderr, "initializing %d diminfo_max widgets",
							n_dims );
		exit( -1 );
		}
	diminfo_units_widget = (Widget *)malloc( (n_dims+1)*sizeof(Widget));
	if( diminfo_units_widget == NULL ) {
		fprintf( stderr, "ncview: x_init_dim_info: malloc failed ");
		fprintf( stderr, "initializing %d diminfo_units widgets",
							n_dims );
		exit( -1 );
		}
	/* Mark the end of the arrays by a 'NULL' */
	*(diminfo_row_widget   + n_dims) = NULL;
	*(diminfo_dim_widget   + n_dims) = NULL;
	*(diminfo_name_widget  + n_dims) = NULL;
	*(diminfo_min_widget   + n_dims) = NULL;
	*(diminfo_cur_widget   + n_dims) = NULL;
	*(diminfo_max_widget   + n_dims) = NULL;
	*(diminfo_units_widget + n_dims) = NULL;

	for( ll=0; ll<n_dims; ll++ )
		{
                snprintf( widget_name, 127, "diminfo_row_%1ld", ll );
                if( ll == 0 )
			*(diminfo_row_widget+ll) = XtVaCreateManagedWidget(
				widget_name,
				boxWidgetClass,
				commandcanvas_widget,
				XtNorientation, XtorientHorizontal,
				XtNfromVert, labels_row_widget,
				XtNheight, bb_height,
				XtNwidth,  bb_width,
				NULL);
		else
			*(diminfo_row_widget+ll) = XtVaCreateManagedWidget(
				widget_name,
				boxWidgetClass,
				commandcanvas_widget,
				XtNorientation, XtorientHorizontal,
				XtNfromVert, *(diminfo_row_widget + (ll-1)),
				XtNheight, bb_height,
				XtNwidth,  bb_width,
				NULL);

		snprintf( widget_name, 127, "diminfo_dim_%1ld", ll );
		*(diminfo_dim_widget+ll) = XtVaCreateManagedWidget(
			widget_name,
			labelWidgetClass,
			*(diminfo_row_widget+ll),
			XtNlabel, "",
			XtNjustify, XtJustifyRight,
			XtNwidth, 50,
			XtNborderWidth, 0,
			NULL);

		snprintf( widget_name, 127, "diminfo_name_%1ld", ll );
		*(diminfo_name_widget+ll) = XtVaCreateManagedWidget(
			widget_name,
			labelWidgetClass,
			*(diminfo_row_widget+ll),
			XtNlabel, dims->string,
			XtNwidth, app_data.dimlabel_width,
			XtNborderWidth, 0,
			NULL);

		snprintf( widget_name, 127, "diminfo_min_%1ld", ll );
		*(diminfo_min_widget+ll) = XtVaCreateManagedWidget(
			widget_name,
			labelWidgetClass,
			*(diminfo_row_widget+ll),
			XtNlabel, "Min:",
			XtNwidth, app_data.dimlabel_width,
			XtNborderWidth, 0,
			NULL);

		snprintf( widget_name, 127, "diminfo_cur_%1ld", ll );
		*(diminfo_cur_widget+ll) = XtVaCreateManagedWidget(
			widget_name,
			commandWidgetClass,
			*(diminfo_row_widget+ll),
			XtNlabel, "Current:",
			XtNwidth, app_data.dimlabel_width,
			NULL);
        	XtAddCallback( *(diminfo_cur_widget+ll), XtNcallback, 
      					diminfo_cur_mod1, (XtPointer)ll );

		/* Add the modifications for the created curr_dimension button;
		 * Button-3 gives mod3 and holding down control gives mod2.
		 */
		XtAugmentTranslations( *(diminfo_cur_widget+ll), 
			XtParseTranslationTable( 
				"<Btn3Down>,<Btn3Up>: diminfo_cur_mod3()" ));
		XtOverrideTranslations( *(diminfo_cur_widget+ll), 
			XtParseTranslationTable( 
				"Ctrl<Btn1Down>,<Btn1Up>: diminfo_cur_mod2()" ));
		XtOverrideTranslations( *(diminfo_cur_widget+ll), 
			XtParseTranslationTable( 
				"Ctrl<Btn3Down>,<Btn3Up>: diminfo_cur_mod4()" ));

		snprintf( widget_name, 127, "diminfo_max_%1ld", ll );
		*(diminfo_max_widget+ll) = XtVaCreateManagedWidget(
			widget_name,
			labelWidgetClass,
			*(diminfo_row_widget+ll),
			XtNlabel, "Max:",
			XtNwidth, app_data.dimlabel_width,
			XtNborderWidth, 0,
			NULL);

		snprintf( widget_name, 127, "diminfo_units_%1ld", ll );
		*(diminfo_units_widget+ll) = XtVaCreateManagedWidget(
			widget_name,
			labelWidgetClass,
			*(diminfo_row_widget+ll),
			XtNlabel, "Units:",
			XtNwidth, app_data.dimlabel_width,
			XtNborderWidth, 0,
			NULL);

		dims = dims->next;
		}
}	

/*************************************************************************************************/
void x_clear_dim_info()
{
	Widget	w;
	int	i;

	if( diminfo_row_widget == NULL )
		return;

	/* Many thanks to Marco Atzeri for fixing a bug that was here. Upshot is that
	 * the widgets appear to be linked, so safer to delete from the tail back to
	 * the head rather than from the head to the tail.
	 */
	i=0;
	while( (w = *(diminfo_row_widget + i)) != NULL )
		i++;

	while( i>0 ) {
		i--;
		XtDestroyWidget( *(diminfo_row_widget + i ));
		}
}

/*************************************************************************************************/
void x_set_cursor_busy()
{
	if( busy_cursor == (Cursor)NULL )
		return;

	XDefineCursor( XtDisplay( commandcanvas_widget ),
	   	    XtWindow( commandcanvas_widget ), busy_cursor );

	
	if( ccontour_popped_up )
		XDefineCursor( XtDisplay( ccontour_widget ),
	   	    XtWindow( ccontour_widget ), busy_cursor );

	if( dimsel_popup_widget != NULL )
		XDefineCursor( XtDisplay( ccontour_widget ),
	   	    XtWindow( dimsel_popup_widget ), busy_cursor );

	XFlush( XtDisplay( commandcanvas_widget ));
}
	
/*************************************************************************************************/
void x_set_cursor_normal()
{
	if( busy_cursor == (Cursor)NULL )
		return;

	XUndefineCursor( XtDisplay( commandcanvas_widget ),
	   	    XtWindow( commandcanvas_widget ) );

	if( ccontour_popped_up )
		XUndefineCursor( XtDisplay( ccontour_widget ),
	   	    XtWindow( ccontour_widget ) ); 

	if( dimsel_popup_widget != NULL )
		XUndefineCursor( XtDisplay( ccontour_widget ),
	   	    XtWindow( dimsel_popup_widget ) );

}

/*************************************************************************************************/
void x_set_windows_colormap_to_current( Widget w )
{
	XSetWindowColormap( XtDisplay(topLevel), XtWindow(w), current_colormap );
}
	
/*************************************************************************************************/
void x_get_window_position( int *llx, int *lly, int *urx, int *ury )
{
	Dimension	width, height;
	Position	x, y;

	XtVaGetValues    ( commandcanvas_widget, XtNwidth,  &width, 
					XtNheight, &height, NULL );
	XtTranslateCoords( commandcanvas_widget, (Position)width,
					(Position)height, &x, &y );
	*urx = (int)x;
	*ury = (int)y;
	XtTranslateCoords( commandcanvas_widget, (Position)0,
					(Position)0, &x, &y );
	*llx = (int)x;
	*lly = (int)y;
}

/*************************************************************************************************/
void pix_to_rgb( ncv_pixel pix,  int *r, int *g, int *b )
{
	*r = (current_colormap_list->color_list+pix)->red;
	*g = (current_colormap_list->color_list+pix)->green;
	*b = (current_colormap_list->color_list+pix)->blue;
}

/*************************************************************************************************/
void x_popup_2d_window()
{
	Position  root_x, root_y;
	Dimension width, height;
	static int 	first_time = 1;

	if( first_time ) {
		/* Move the ccontourpanel to a reasonable location */
		XtVaGetValues    ( commandcanvas_widget, XtNwidth,  &width, XtNheight, &height, NULL );
		XtTranslateCoords( commandcanvas_widget, (Position)1, (Position)height, &root_x, &root_y );
		XtVaSetValues    ( ccontourpanel_widget, XtNx, root_x, XtNy, root_y, NULL );
		first_time = 0;
		}

	XtPopup( ccontourpanel_widget, XtGrabNone );

	ccontour_popped_up = TRUE;
}

/*************************************************************************************************/
void x_popdown_2d_window()
{
	XtPopdown( ccontourpanel_widget );
	ccontour_popped_up = FALSE;
}

/*************************************************************************************************/
void expose_ccontour( Widget w, XtPointer client_data, XExposeEvent *event, Boolean *continue_to_dispatch )
{
	if( event->type != Expose ) {
		fprintf( stderr, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n" );
		fprintf( stderr, "Got a non-expose event to expose_ccontour!\n" );
		fprintf( stderr, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n" );
		return;
		}

	if( (event->count == 0) && (event->width > 1) && (event->height > 1))
		view_draw( TRUE, FALSE );
}

/*************************************************************************************************/
void expose_colorbar( Widget w, XtPointer client_data, XExposeEvent *event, Boolean *continue_to_dispatch )
{
	if( event->type != Expose ) {
		fprintf( stderr, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n" );
		fprintf( stderr, "Got a non-expose event to expose_colorbar!\n" );
		fprintf( stderr, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n" );
		return;
		}

	if( (event->count == 0) && (event->width > 1) && (event->height > 1))
		x_draw_colorbar();
}

/*************************************************************************************************/
void 	varsel_menu_select(Widget w, XtPointer client_data, XtPointer call_data )
{
	NCVar 	*var;
	int 	i, menu_item;
	long	l_client_data;

	l_client_data = (long)client_data;
	menu_item = (int)l_client_data;

	var = variables;
	for( i=0; i<menu_item; i++ )
		var = var->next;

	in_variable_selected( var->name );
}

/*************************************************************************************************/
void 	testf(Widget w, XButtonEvent *e, String *p, Cardinal *n )
{
	printf( "in testf!\n" );
}

/*************************************************************************************************/
int x_report_auto_overlay( void )
{
	return( app_data.auto_overlay );
}

/*************************************************************************************************/
void x_create_colorbar( float user_min, float user_max, int transform )
{
	int width, height, toplev_width;

	if( options.debug ) fprintf( stderr, "x_create_colorbar: entering\n" );
	toplev_width = x_report_widget_width(NULL);
	if( options.debug ) fprintf( stderr, "x_create_colorbar: toplev_width=%d\n", toplev_width );

	x_set_colormap_form_width(  toplev_width-10 );

	if( options.debug ) fprintf( stderr, "x_create_colorbar: just set colormap width to %d\n",
		toplev_width-10 );

	width  = x_report_widget_width(&colorbar_widget);
	height = CBAR_HEIGHT;

	if( options.debug ) fprintf( stderr, "x_create_colorbar: about to call cbar_make with width=%d height=%d n_extra_colors=%d user_min=%f user_max=%f transform=%d\n",
		width, height, options.n_extra_colors, user_min, user_max, transform );
	cbar_make( width, height, options.n_extra_colors, user_min, user_max, transform );
	if( options.debug ) fprintf( stderr, "x_create_colorbar: exiting\n" );
}

/*************************************************************************************************/
void x_draw_colorbar()
{
	Display	*display;
	Screen	*screen;
	static XImage	*ximage;
	XGCValues values;
	GC	gc;
	static	size_t last_width=0L, last_height=0L;
	static 	unsigned char *tc_data=NULL;
	unsigned char	*data;
	int	width, height;

	if( options.debug ) fprintf( stderr, "x_draw_colorbar: entering\n" );

	/* This can happen when things are first being drawn, that there is
	 * an initial expose event even before the colorbar has been made 
	 */
	if( cbar_info( &data, &width, &height ) < 0 ) {
		if( options.debug ) fprintf( stderr, "x_draw_colorbar: quick exit\n" );
		return;
		}

	display = XtDisplay( colorbar_widget );
	screen  = XtScreen ( colorbar_widget );

	if( options.display_type == TrueColor ) {
		if( options.debug ) fprintf( stderr, "x_draw_colorbar: TrueColor display\n" );
		/* If the TrueColor data array does not yet exist, 
		 * or is the wrong size, then allocate it.
		 */
		if( tc_data == NULL ) {
			tc_data=(unsigned char *)malloc( server.bitmap_unit*width*height );
			last_width  = width;
			last_height = height;
			}
		else if( (width!=last_width) || (height!=last_height)) {
			free( tc_data );
			last_width  = width;
			last_height = height;
			tc_data=(unsigned char *)malloc( server.bitmap_unit*width*height );
			}
		/* Convert data to TrueColor representation, with
		 * the proper number of bytes per pixel
		 */
		make_tc_data( data, width, height, current_colormap_list->color_list, tc_data );

		ximage  = XCreateImage(
			display,
			XDefaultVisualOfScreen( screen ),
			XDefaultDepthOfScreen ( screen ),
			ZPixmap,
			0,
			(char *)tc_data, 
			(unsigned int)width, (unsigned int)height,
			32, 0 );
		}
	else /* display_type == PseudoColor */
		{
		if( options.debug ) fprintf( stderr, "x_draw_colorbar: PseudoColor display\n" );
		ximage  = XCreateImage(
			display,
			XDefaultVisualOfScreen( screen ),
			XDefaultDepthOfScreen ( screen ),
			ZPixmap,
			0,
			(char *)data,
			(unsigned int)width, (unsigned int)height,
			8, 0 );
		}
	if( options.debug ) fprintf( stderr, "x_draw_colorbar: ximage made\n" );

	gc = XtGetGC( ccontour_widget, (XtGCMask)0, &values );

	if( !valid_display ) {
		if( options.debug ) fprintf( stderr, "x_draw_colorbar: display is not valid; returning\n" );
		return;
		}

	XPutImage(
		display,
		XtWindow( colorbar_widget ),
		gc,
		ximage,
		0, 0, 0, 0,
		(unsigned int)width, (unsigned int)height );

	if( options.debug ) fprintf( stderr, "x_draw_colorbar: exiting\n" );
}

/*************************************************************************************************/
/* If w is NULL, then reports the width of the top level widget.
 */
int x_report_widget_width( Widget *w )
{
	int 		retval, x, y;
	unsigned int 	width, height, border_width, depth;
	Drawable	root;
	Widget		*w2use;

	if( w == NULL )
		w2use = &topLevel;
	else
		w2use = w;
	
	if( ! XtIsRealized(*w2use))
		return(-1);

	if( XGetGeometry( XtDisplay(*w2use), XtWindow(*w2use),
			&root, &x, &y, &width, &height, &border_width, &depth) == False )
		return(-1);

	retval = (int)width;
	return(retval);
}

/*************************************************************************************************/
void x_set_colormap_form_width( int width )
{
	XtVaSetValues( colorbar_form_widget, XtNwidth, width, NULL );
}

#ifdef HAVE_PNG
/*************************************************************************************************/
void dump_to_png( unsigned char *data, size_t width, size_t height, size_t frameno )
{
	char		filename[2048];
	FILE		*out_file;
	png_structp	png_ptr;
	png_infop	info_ptr;
	png_colorp	palette;
	int		bit_depth, i, j, n_palette;
	unsigned char	**row_pointers;
	static int	error_state = 0;

	if( error_state == 1 ) return;

	snprintf( filename, 2047, "frame.%05ld.png", frameno );
	frameno++;

	/* Open binary output file */
	if( (out_file = fopen( filename, "wb" )) == NULL ) {
		fprintf( stderr, "ncview: can't open PNG file %s for writing\n", filename );
		error_state = 1;
		return;
		}

	/* Set up PNG information structures */
	png_ptr = png_create_write_struct( PNG_LIBPNG_VER_STRING, (png_voidp)NULL, NULL, NULL );	/* use default error handlers */
	if( ! png_ptr ) {
		fprintf( stderr, "Error returned when trying to allocate PNG write structure\n" );
		error_state = 1;
		return;
		}

	info_ptr = png_create_info_struct( png_ptr );
	if( ! info_ptr ) {
		fprintf( stderr, "Error returned when trying to allocate PNG info structure\n" );
		png_destroy_write_struct( &png_ptr, (png_infopp)NULL );
		error_state = 1;
		return;
		}

	/* Set up PNG I/O ops */
	png_init_io( png_ptr, out_file );

	/* Describe our image for PNG */
	bit_depth = 8;
	png_set_IHDR( png_ptr, info_ptr, width, height, bit_depth, 
		PNG_COLOR_TYPE_PALETTE,		/* ncview operates with palettes almost exclusively */
		PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT,
		PNG_FILTER_TYPE_DEFAULT );

	/* Set our palette */
	n_palette = 256;
	palette = (png_colorp)malloc( sizeof(png_color) * n_palette );
	if( palette == NULL ) {
		fprintf( stderr, "Failed to allocate colormap; returning without writing output file\n" );
		error_state = 1;
		return;
		}
	for( i=0; i<n_palette; i++ ) {
		palette[i].red   = ((current_colormap_list->color_list+i)->red   >> 8);
		palette[i].green = ((current_colormap_list->color_list+i)->green >> 8);
		palette[i].blue  = ((current_colormap_list->color_list+i)->blue  >> 8);
		}
	png_set_PLTE( png_ptr, info_ptr, palette, n_palette );

	/* Make our row pointers */
	row_pointers = (unsigned char **)malloc( sizeof( unsigned char * ) * height );
	if( row_pointers == NULL ) {
		fprintf( stderr, "Failed to allocate row pointers; returning without writing output file\n" );
		error_state = 1;
		return;
		}
	for( j=0; j<height; j++ ) 
		row_pointers[j] = data + j*width;

	/* Associate row pointers with the PNG structures */
	png_set_rows( png_ptr, info_ptr, row_pointers );

	/* Set up longjump target (PNG will use this for an error) */
	if( setjmp( png_jmpbuf(png_ptr))) {
		fprintf( stderr, "Error returned when trying to write PNG file, aborting operation\n" );
		png_destroy_write_struct( &png_ptr, &info_ptr );
		fclose( out_file );
		error_state = 1;
		return;
		}

	/* Write PNG data */
	png_write_png( png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL );

	/* Delete allocated space */
	free( row_pointers );
	free( palette );

	fclose(out_file);
}
#endif

