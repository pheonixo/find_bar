#ifndef _LCI_FINDPORT_H_
#define _LCI_FINDPORT_H_

#include <gtk/gtk.h>
#include <math.h>  // M_PI
 double fabs(double x);
#include <ctype.h>
#include "memrchr.c"

static GtkClipboard* clipboard = NULL;

// using 2 windows, instead of findbar opening in same
// window as textview. Allows me to work on 'text_buffer'.
#define TW 800
#define TH 200  // 200
#define BOX_WIDTH  600

#define DEBUGGING 0
#define USE_MARKS 1
#define VERIFY    0
#define MARK_ALLOC  4096
#define TGAP_ALLOC  4096
// used for draw_buffer
#define BUFF_ALLOC  8192
#define BOX_HEIGHT 22
#define BUTTON_TEXT_MIN 3
/* increased to 32 objects for testing */
#define OBJECTS_MAX  32
//#define FONT_NAME "Pearl"
#define FONT_NAME "Sans"
//#define FONT_NAME "Noto Sans"
//#define FONT_NAME "Monospace"
// motion too sensitive, need for fast, slight motion disqualifier
// case: auto-scroll when you think there's no motion
static double press_event_x;
static double press_event_y;
//                                        Fh,FRh,bbm
static char window_adjustments[2][3] = { { 0, -3, 2 },  // <= 20
                                         { 0, -4, 3 }   //  > 20
};


typedef enum {
 PHX_NONE     = 0,
 PHX_ANY      = 1,
 PHX_IFACE,
 PHX_BUTTON,           // base type (obj)
 PHX_BUTTON_LABELED,   // base type (obj + (child == label))
 PHX_BUTTON_COMBO,     // button variation (obj + several children)
                       //   event distinct
 PHX_NAVIGATE_LEFT,    // base type (obj) altered 'button draw'
                       //   event distinct
 PHX_NAVIGATE_RIGHT,   // base type (obj) altered 'button draw'
                       //   event distinct
 PHX_TEXTVIEW,         // base type (obj)
                       //   events/auto-scroll, multi-line viewport
 PHX_ENTRY,            // base type (obj)  textview variation
                       //   events/auto-scroll, single line viewport
 PHX_LABEL,            // base type (obj)  textview variation
                       //   non-events/non-scroll, can be multiline
 PHX_DRAWING,          // base type (obj)
 PHX_TEXTBUFFER,       // a non-drawing textview
 PHX_POPUP, //?
 PHX_OBJECT_LAST
} PhxObjectType;

typedef struct _PhxInterface           PhxInterface;
typedef struct _PhxObject              PhxObject;
typedef struct { int x, y, w, h; }     PhxRectangle;
typedef void (*PhxDrawHandler)(PhxObject*, cairo_t*);
typedef struct _PhxObject              PhxObjectDrawing;
typedef struct _PhxObjectTextview      PhxObjectTextview;
typedef struct _PhxObjectTextview      PhxObjectLabel;
typedef struct _PhxObjectButton        PhxObjectButton;
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
 int            state;                   // flags/state
 PhxRectangle   mete_box;                // allocate box
 GdkWindow      *parent_window;
 PhxObject     **object_list;       // list of alloted base objects
 char          (*event_map)[2048];  // mapping of event areas  
 size_t          map_size;          // realloc
 PhxObject      *has_focus;         // active object within iface
   // expect attachments
 void           *reserved[5];
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
 int            state;                   // flags/state
 PhxRectangle   mete_box;                // allocate box
 GdkWindow      *parent_window;
 PhxObject     **object_list;       // list of alloted base objects
 char          (*event_map)[2048];  // mapping of event areas  
 size_t          map_size;          // realloc
 PhxObject      *has_focus;         // active object within iface
   // addons that make findport different than PhxInterface
 void           *param_data;        // passed to iface created/called by self
 void           *session;           // called void for demo purposes
 int              nfsearch;
 struct _fsearch *fsearchs;
} LCIFindPort;

LCIFindPort *findPort = NULL;

struct _PhxObject {
 PhxObjectType  type;
 int            state;                   // flags/state
 PhxRectangle   mete_box;                // allocate box
 PhxRectangle   draw_box;                // clip box
 PhxDrawHandler _draw_cb;                // how to draw
 PhxObject     *child;                   // non-specific addon
 PhxInterface  *iface;                   // object mounting
};

struct _PhxObjectButton {
 PhxObjectType  type;
 int            state;                   // flags/state
 PhxRectangle   mete_box;                // allocate box
 PhxRectangle   draw_box;                // clip box
 PhxDrawHandler _draw_cb;                // how to draw
 PhxObjectTextview *label;               // textview addon
 PhxInterface  *iface;                   // object mounting
};

typedef struct { int x, y, offset; } location;
typedef struct _mark   PhxMark;

// non-drawing object, character manipulation
struct _PhxObjectTextbuffer {
 PhxObjectType  type;
 int            state;                   // flags/state
 PhxRectangle   mete_box;                // ignored
 void *string_mete; // buffer
 char *string;      // c-str pointer, ref to string_mete
 int str_nil;       // simular to strlen return
 int gap_start, gap_end, gap_delta;  // editing management
  // lowest offset of gap_start, where edit began or is (<backspace>)
 int dirtyo;
   // use as both edit offset, and draw of cursor
 location insert, release, interim, drop;
};

struct _PhxObjectTextview {
 PhxObjectType  type;
 int            state;                   // flags/state
 PhxRectangle   mete_box;                // allocate box
 PhxRectangle   draw_box;                // clip box
 PhxDrawHandler _draw_cb;                // how to draw
 PhxObject     *child;                   // non-specific addon
 PhxInterface  *iface;                   // object mounting
  // textbuffer
 void *string_mete; // buffer
 char *string;      // c-str pointer, ref to string_mete
 int str_nil;       // simular to strlen return
 int gap_start, gap_end, gap_delta;       // editing management
  // lowest offset of gap_start, where edit began or is (<backspace>)
 int dirtyo;
 location insert, release, interim, drop; // cursor management
  // all following are textbuffer drawing when placed into a textview
 PhxRectangle bin;   // scrolled view of text, start of displayed text
  // buffer (copy of string inside bin) used to avoid 'lock'(s)
 char *draw_buffer;
  // attributes
 int font_em;
 char *font_name;
 double font_size;
 double font_origin;
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
