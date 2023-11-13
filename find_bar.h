#ifndef _LCI_FINDPORT_H_
#define _LCI_FINDPORT_H_

#include <gtk/gtk.h>
#include <math.h>  // M_PI
 double fabs(double x);
#include <ctype.h>
#include "memrchr.c"

#define USE_MARKS   1
#define OBJS_ALLOC  16
#define OBJS_PWR    4

#define MARK_ALLOC  4096
#define TGAP_ALLOC  4096
// used for draw_buffer
#define BUFF_ALLOC  8192
#define BUTTON_TEXT_MIN 3

#define BOX_HEIGHT 22
//#define FONT_NAME "Pearl"
#define FONT_NAME "Sans"
//#define FONT_NAME "Noto Sans"
//#define FONT_NAME "Monospace"

static GtkClipboard* clipboard = NULL;
// motion too sensitive, need for fast, slight motion disqualifier
// case: auto-scroll when you think there's no motion
static double press_event_x;
static double press_event_y;
//                                        Fh,FRh,bbm
static char window_adjustments[2][3] = { { 0, -3, 2 },  // <= 20
                                         { 0, -4, 3 }   //  > 20
};

// Evolution/De-evolution of PhxOject tends to be:
//  type, state, mete_box (usage, variations, space occupies)

typedef enum {
  /* Non-drawables */
 PHX_NONE     = 0,
 PHX_ANY      = 1,
 PHX_IFACE,            // base type (obj) connection to gdk/gtk
 PHX_TEXTBUFFER,       // a non-drawing text (evolving)
  /* Drawable, normally straight cairo commands */
 PHX_DRAWING,          // base type (obj)
  /* Drawables Text */
 PHX_TEXTVIEW,         // base type (obj)
                       //   events/auto-scroll, multi-line viewport
 PHX_ENTRY,            // base type (obj)  textview variation
                       //   events/auto-scroll, single line viewport
 PHX_LABEL,            // base type (obj)  textview variation
                       //   non-events/non-scroll, can be multiline
 PHX_COMBO_LABEL,      // base type (obj)  textview variation
                       //   non-edit, can be multiline, has bank
  /* Drawables Buttons (combines with text and/or drawing) */
 PHX_BUTTON,           // base type (obj)
 PHX_BUTTON_LABELED,   // base type (obj + (child == label))
 PHX_BUTTON_COMBO,     // button variation (obj + several children)
                       //   event distinct
 PHX_NAVIGATE_LEFT,    // base type (obj) altered 'button draw'
                       //   event distinct
 PHX_NAVIGATE_RIGHT,   // base type (obj) altered 'button draw'
                       //   event distinct
  /* Working on */
 PHX_POPUP,            //  (evolving) passes bank width/height and
                       // child linkage to button/label
 PHX_BANK_MENU,        // bank doesn't overwrite actuator object
 PHX_BANK_COMBO,       // bank is part of actuator object
 PHX_OBJECT_LAST
} PhxObjectType;

typedef struct _PhxInterface           PhxInterface;
typedef _Bool (*PhxConfigureHandler)(PhxInterface *, GdkEvent *, void *);
typedef struct _PhxObject              PhxObject;
typedef struct { int x, y, w, h; }     PhxRectangle;
typedef void (*PhxDrawHandler)(PhxObject*, cairo_t*);
typedef struct _PhxObject              PhxObjectDrawing;
typedef struct _PhxObjectTextview      PhxObjectTextview;
typedef struct _PhxObjectLabel         PhxObjectLabel;
typedef struct _PhxObjectButton        PhxObjectButton;
typedef struct _PhxBank                PhxBank;
typedef void (*PhxActionHandler)(PhxInterface *, GdkEvent *, PhxObject *);
typedef void (*PhxResultHandler)(PhxBank *);
typedef struct _PhxObjectTextbuffer    PhxObjectTextbuffer;

typedef enum {
 MOUSE_CLEAR          = 0,
 MOUSE_MOTION         = 1,
 MOUSE_1BUTTON        = 2,
 MOUSE_1BUTTON_MOTION = 3,
 MOUSE_DND            = 7, // is 1BUTTON_MOTION, but different handler
 DND_MOVE_CURSOR      = 7,
 DND_COPY_CURSOR      = 15
} BState;

struct _PhxInterface {
 PhxObjectType  type;
 int            state;              // flags/state
 PhxRectangle   mete_box;           // allocate box
 PhxRectangle   draw_box;           // clip box
 PhxObject     **objects;           // list of alloted base objects
 char           *event_map;         // mapping of event areas
 PhxObject      *has_focus;         // active object within iface
 GdkWindow      *parent_window;     // the content window of the 'draw'
 PhxConfigureHandler _configure_event;
   // expect attachments
 void           *reserved[5];
};

// PHX_BANK_MENU, PHX_BANK_COMBO
// Note: design for 65535 objects, semi-related consideration is OBJS_ALLOC
// allocation in upper 16 of state
// motion select in upper 16 of display_indicies
// current display_object in lower 16
// at OBJS_ALLOC == 16 need 12bits for allocation
// OBJS_ALLOC min of 2 needs 15bits
struct _PhxBank {
 PhxObjectType  type;
 int            state;              // flags/state
 PhxRectangle   mete_box;           // allocate box
 PhxRectangle   draw_box;           // clip box
 PhxObject     **objects;           // list of alloted base objects
 char           *event_map;         // mapping of event areas
 PhxObject      *has_focus;         // active object within bank
 GdkWindow       *parent_window;     // the content window of the 'draw'
 PhxConfigureHandler _configure_event;
   // expect attachments
 PhxObjectButton *actuator;          // activates popup
 PhxObject       *display_object;    // the translated bank member for actuator
 int             display_indicies;   // the highlit:current mapping
 int             flag;
 PhxResultHandler _result_cb;        // TRUE if display_object changes
                                     // allows configure alters on popup complete
 void            *reserved[1];
};

/* Application Global, used for searching text */
// Note: LCIFindPort can be cast as PhxInterface
// LCIFindPort is PhxInterface with addons

struct _results {
 int qbits;
 int qoffsets[32];
};

struct _fsearch {
 char *qfile;  // allocated filename results found for
 char *search_string;  // allocated c-string data obtained for
 struct _results *q_results;  // allocated circular key data
 int n_qr;          // sizeof(q_results)/sizeof(struct _results)
 int qdx;           // current index accessing q_results
   // a signal link for gtk's textbuffer to set dirtyo
   // keep? should remove on close file menu activate
   // not needed when using PhxOjectTextview
 unsigned long   changed_id;
 int dirtyo;
};

typedef struct _LciFindPort {
 PhxObjectType  type;
 int            state;              // flags/state
 PhxRectangle   mete_box;           // allocate box
 PhxRectangle   draw_box;           // clip box
 PhxObject     **objects;           // list of alloted base objects
 char           *event_map;         // mapping of event areas
 PhxObject      *has_focus;         // active object within iface
 GdkWindow      *parent_window;     // the content window of the 'draw'
 PhxConfigureHandler _configure_event;
   // expect attachments
   // addons that make findport different than PhxInterface
 void           *param_data;        // passed to iface created/called by self
 void           *session;           // called void for demo purposes
 int              nfsearch;
 struct _fsearch *fsearchs;
} LCIFindPort;

LCIFindPort *findPort = NULL;

struct _PhxObject {
 PhxObjectType  type;
 int            state;              // flags/state
 PhxRectangle   mete_box;           // allocate box
 PhxRectangle   draw_box;           // clip box
 PhxDrawHandler _draw_cb;           // how to draw
 PhxObject      *child;             // non-specific addon
 PhxInterface   *iface;             // object mounting
};

struct _PhxObjectButton {
 PhxObjectType  type;
 int            state;              // flags/state
 PhxRectangle   mete_box;           // allocate box
 PhxRectangle   draw_box;           // clip box
 PhxDrawHandler _draw_cb;           // how to draw
 PhxObject      *child;             // non-specific addon
 PhxInterface   *iface;             // object mounting
 PhxActionHandler  _event_cb;       // button press/release actions
 PhxBank        *bank;              // special button list
};

struct _PhxObjectLabel {
 PhxObjectType  type;
 int            state;              // flags/state
 PhxRectangle   mete_box;           // allocate box
 PhxRectangle   draw_box;           // clip box
 PhxDrawHandler _draw_cb;           // how to draw
 PhxObject      *child;             // non-specific addon
 PhxInterface   *iface;             // object mounting
 char *string;                      // c-str pointer
  // attributes
 int font_em;
 char *font_name;
 double font_size;
 double font_origin;
};

typedef struct { int x, y, offset; } location;

// non-drawing object, character manipulation
struct _PhxObjectTextbuffer {
 PhxObjectType  type;
 int            state;                   // flags/state
 PhxRectangle   mete_box;                // ignored
// void *string_mete; // buffer
 char *string;      // c-str pointer, ref to string_mete
 int str_nil;       // simular to strlen return
 int gap_start, gap_end, gap_delta;  // editing management
  // lowest offset of gap_start, where edit began or is (<backspace>)
 int dirtyo;
   // use as both edit offset, and draw of cursor
 location insert, release, interim, drop;
};

typedef struct _mark   PhxMark;

struct _PhxObjectTextview {
 PhxObjectType  type;
 int            state;              // flags/state
 PhxRectangle   mete_box;           // allocate box
 PhxRectangle   draw_box;           // clip box
 PhxDrawHandler _draw_cb;           // how to draw
 PhxObject      *child;             // non-specific addon
 PhxInterface   *iface;             // object mounting
 char *string;      // c-str pointer, ref to string_mete

  // attributes
 int font_em;
 char *font_name;
 double font_size;
 double font_origin;

 int str_nil;       // simular to strlen return
 int gap_start, gap_end, gap_delta;       // editing management
  // lowest offset of gap_start, where edit began or is (<backspace>)
 int dirtyo;
 location insert, release, interim, drop; // cursor management
  // all following are textbuffer drawing when placed into a textview
 PhxRectangle bin;   // scrolled view of text, start of displayed text
  // buffer (copy of string inside bin) used to avoid 'lock'(s)
 char *draw_buffer;
 char *glyph_widths;

#if USE_MARKS
 PhxMark *newline_list;
   // editing management
 int list_nil, list_size;
 _Bool list_ldelta; // when <newline> insert/delete force an update
 int list_dirtyo;   // when edit is on single line
#endif
};

  // find window object naming, in order of creation
  // or index naming of object_list[]
enum {
 fport = 0,
 choose_box = 1,
 replace_all_box,
 replace_box,
 replace_find_box,
 close1_box,
 close0_box,
 textview_replace_box,
 textview_find_box,
 navigate_right_box,
 navigate_left_box,
 found_box,
 last_object
};

enum {
 RESULT_EXACT = 0,
 RESULT_LEFT,
 RESULT_RIGHT
};

/*
    Updating marks should occur after editing. This means after
  the position of the edit location is moved, whether by mouse
  click or key movement press(s).
    Thinking 'void *data' should be a data handler instead. Or
  at minimum, both.
*/

#if USE_MARKS
enum _phxmarks {
 PHXLINE = 1,
 PHXSEARCH,
 PHXLASTMARK
};
typedef enum _phxmarks PhxMarkType;
struct _mark {
 PhxMarkType  type;
 int    offset;
 int    line;  // can use as other when different than newline
 void  *data;
// need data change function pointer
};
#endif

#define SHIFT 8
#define VISUAL (SHIFT + 8)

#define HXPD_NONE (0x00000000 << (SHIFT + 0))
#define HXPD_LFT  (0x00000001 << (SHIFT + 0))
#define HXPD_RGT  (0x00000002 << (SHIFT + 0))
#define HXPD_BOTH (0x00000003 << (SHIFT + 0))
#define HXPD_MSK  (0x00000003 << (SHIFT + 0))

#define VXPD_NONE (0x00000000 << (SHIFT + 2))
#define VXPD_TOP  (0x00000001 << (SHIFT + 2))
#define VXPD_BTM  (0x00000002 << (SHIFT + 2))
#define VXPD_BOTH (0x00000003 << (SHIFT + 2))
#define VXPD_MSK  (0x00000003 << (SHIFT + 2))

#define HJST_LFT  (0x00000000 << (SHIFT + 4))
#define HJST_RGT  (0x00000001 << (SHIFT + 4))
#define HJST_CTR  (0x00000002 << (SHIFT + 4))
#define HJST_MSK  (0x00000003 << (SHIFT + 4))

#define VJST_TOP  (0x00000000 << (SHIFT + 6))
#define VJST_BTM  (0x00000001 << (SHIFT + 6))
#define VJST_CTR  (0x00000002 << (SHIFT + 6))
#define VJST_MSK  (0x00000003 << (SHIFT + 6))

void    lci_findport_receiver_text(LCIFindPort *, char, char *, size_t);
void    lci_findport_search(LCIFindPort *);
void    lci_findport_clear_results(LCIFindPort *);

#endif //_LCI_FINDPORT_H_
