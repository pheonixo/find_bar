#ifndef _LCI_FINDPORT_H_
#define _LCI_FINDPORT_H_

//                                        Fh,FRh,bbm
static char window_adjustments[2][3] = { { 0, -3, 2 },  // <= 20
                                         { 0, -4, 3 }   //  > 20
};

#define BOX_HEIGHT 22

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
 RESULT_EQU = 0,
 RESULT_LT,
 RESULT_GT,
 RESULT_LE,
 RESULT_GE
};

struct _search_data {
 int          n_pairs;
 PhxMarkData  *pairs;
};
struct _fsearch {
 char *sfile; // id
 char *sstring; // results for this, a sub-id
 int  sdx;    // current/last selected result
 int  sdirtyo; // location start of update needs
 struct _search_data *sdata;
 unsigned long   changed_id; // hook, for gtk, maybe others?
};

void    lci_findport_receiver_text(LCIFindPort *, char, char *, size_t);
void    lci_findport_search(LCIFindPort *);
void    lci_findport_clear_results(LCIFindPort *);

#endif //_LCI_FINDPORT_H_
