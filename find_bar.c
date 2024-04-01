#include "phxobjects.c"
#include "find_bar.h"

#pragma mark *** FindPort ***

GtkWidget *main_window = NULL;

static void
result_cb(PhxBank *obank) {

  PhxObjectButton *obtn = (PhxObjectButton*)obank->actuator;
    // base on if 'Find & Replace' is selected
  _Bool set = (obank->display_indices & 0x0FFFFU) == 1;

  visible_set(findPort->objects[close0_box], !set);
  visible_set(findPort->objects[replace_all_box], set);
  visible_set(findPort->objects[replace_box], set);
  visible_set(findPort->objects[replace_find_box], set);
  visible_set(findPort->objects[close1_box], set);
  visible_set(findPort->objects[textview_replace_box], set);

    // set viewable size, one or two rows
  int min_width = (int)((double)BOX_HEIGHT / .0406);
  int idx = (BOX_HEIGHT > 20);
  int two_row_height = (BOX_HEIGHT * 2) + window_adjustments[idx][1];
  GtkWindow *window = GTK_WINDOW(main_window);
  if (set) {
    if (findPort->mete_box.h != two_row_height)
      gtk_window_resize(window, findPort->mete_box.w, two_row_height);
      gtk_widget_set_size_request(main_window, min_width, two_row_height);
  } else {
    if (findPort->mete_box.h == two_row_height)
      gtk_window_resize(window, findPort->mete_box.w, BOX_HEIGHT);
      gtk_widget_set_size_request(main_window, min_width, BOX_HEIGHT);
  }
}

static void
text_buffer_apply_search_tag(PhxObjectTextview *otxt,
                                               struct _fsearch *search) {

  if (search->sdata->n_pairs == 0)  return;
  PhxMarkData *mdPtr = &search->sdata->pairs[search->sdx];

  otxt->insert.offset = mdPtr->d0;
  location_for_offset(otxt, &otxt->insert);
  location_auto_scroll(otxt, &otxt->insert);
  otxt->interim.x = otxt->insert.x;
  otxt->interim.y = otxt->insert.y;
  otxt->interim.offset = otxt->insert.offset;

  otxt->release.offset = mdPtr->d1;
  location_for_offset(otxt, &otxt->release);

  cairo_region_t *crr;
  crr = cairo_region_create_rectangle(
           (cairo_rectangle_int_t *)&otxt->mete_box);
  gdk_window_invalidate_region(otxt->iface->parent_window, crr, FALSE);
  cairo_region_destroy(crr);
}

// with demo passes textview, LCode uses gtk, pass GtkTextBuffer *tbuf instead
static int
findport_results_display(LCIFindPort *fport, struct _fsearch *fdata) {

  int found = fdata->sdata->n_pairs;
    // could sprint direct
  char rbuf[32];
  sprintf(rbuf, "%d found matches", found);
  PhxObjectLabel *olbl = (PhxObjectLabel*)fport->objects[found_box];
  ui_label_set(olbl, rbuf, HJST_RGT);

  _Bool set = found > 1;
  if (found == 1) {
    PhxObjectTextview *otxt = (PhxObjectTextview*)fport->param_data;
    set = ((otxt->insert.offset != fdata->sdata->pairs->d0)
           || (otxt->release.offset != fdata->sdata->pairs->d1));
  }
  sensitive_set(fport->objects[navigate_right_box], set);
  sensitive_set(fport->objects[navigate_left_box], set);

  cairo_region_t *crr;
  crr = cairo_region_create_rectangle(
           (cairo_rectangle_int_t *)&fport->mete_box);
  gdk_window_invalidate_region(fport->parent_window, crr, FALSE);
  cairo_region_destroy(crr);
  return found;
}

/* searches from 'start' offset in 'otxt' using 'search'
 * 'search' has the keyword that is to be found.
 * note: otxt, search(valid) existance checked prior to call.
 * start is set by otxt hook, if marks, else must assume entire dirty */
static int
findport_search_from(PhxObjectTextview *otxt,
                             struct _fsearch *search, int start) {

/* must find revising location in search' offsets.
 * start after valid location offset, replacing all new found */
  if (search->sdirtyo < start)  start = search->sdirtyo;
  search->sdirtyo = INT_MAX;

  char *key = search->sstring;
  if (key == NULL)  return 0;

  struct _search_data *sdata = search->sdata;
  PhxMarkData *mdPtr = search->sdata->pairs;
  int acheck = MARK_ALLOC;  // point to check for realloc
  int ndx = 0;           // start of updating in sdata
  if (sdata->n_pairs != 0) {
      // not pristine, establish point to check for realloc
    acheck = (sdata->n_pairs + (MARK_ALLOC - 1)) & ~(MARK_ALLOC - 1);
    if ((start != 0) && (mdPtr->d0 < start)) {
      PhxMarkData *offset_mark = search->sdata->pairs + (sdata->n_pairs - 1);
      if (offset_mark->d0 > start) {

        offset_mark = bsearch(&start, mdPtr, search->sdata->n_pairs,
                              sizeof(PhxMarkData), _text_mark_offsets_compare);
        if (offset_mark->d0 < start)  offset_mark++;
      }
      if (offset_mark->d0 == INT_MAX)
         start = (--offset_mark)->d0;
      ndx = offset_mark - mdPtr;
      mdPtr = offset_mark;
    }
  }

  char *sPtr = otxt->string;
  char *rdPtr = sPtr + start;

  size_t key_len = strlen(key);
  while ((rdPtr = strstr(rdPtr, key)) != NULL) {
    mdPtr->d0 = rdPtr - sPtr;
    mdPtr->d1 = mdPtr->d0 + key_len;
    rdPtr += key_len;
    mdPtr++;
    if ((++ndx) == acheck) {
      mdPtr = _mark_pairs_realloc(&sdata->pairs, ndx);
      if (mdPtr == NULL)  return -1;
      acheck += MARK_ALLOC;
    }
  }
  mdPtr->d0 = INT_MAX;
  mdPtr->d1 = INT_MAX;
  sdata->n_pairs = ndx;
  return ndx;
}

static void
findport_results_reset(struct _fsearch *search) {
    // clear tags, then result data
  if (search->sstring != NULL) {
    free(search->sstring);
    search->sstring = NULL;
  }
  if (search->sdata != NULL) {
    if (search->sdata->n_pairs & ~(MARK_ALLOC - 1)) {
      free(search->sdata->pairs);
      search->sdata->pairs = malloc(MARK_ALLOC * sizeof(PhxMarkData));
    }
    memset(search->sdata->pairs, INT_MAX, (MARK_ALLOC * sizeof(PhxMarkData)));
    search->sdata->n_pairs = 0;
  }
  search->sdx = 0;
  search->sdirtyo = INT_MAX;
  //changed_id
}

/*
   Pulls up a 'fsearch' data for a given 'filename'.
   Should there be an existing search, the following happens.
   In the case of using 'MARKS', will update 'fsearch' from
   a given offset set by any editing of 'filename's buffer.
   Without 'MARKS', will perform search on entire buffer, assuming
   something changed, and no notice of change can be provided.
*/
static struct _fsearch *
findport_results_get_for(LCIFindPort *fport, char *filename) {

  struct _fsearch *search = fport->fsearchs;
  if (search == NULL) {
    fport->nfsearch = 0;
    fport->fsearchs = (search = malloc(sizeof(struct _fsearch)));
qfile_create:
    search->sfile = NULL;
    if (filename != NULL)
      search->sfile = strdup(filename);
    search->sstring = NULL;
    search->sdx = 0;
    search->sdirtyo = INT_MAX;
    search->sdata = malloc(sizeof(struct _search_data));
    search->sdata->n_pairs = 0;
    search->sdata->pairs = malloc(MARK_ALLOC * sizeof(PhxMarkData));
    memset(search->sdata->pairs, INT_MAX, (MARK_ALLOC * sizeof(PhxMarkData)));
    search->changed_id = 0;
    return search;
  }
  int idx = 0;
  char *fName = filename;
  do {
    char *sName = (&search[idx])->sfile;
    if ((sName == NULL) && (fName == NULL))  break;
    if ((sName != NULL) && (fName != NULL))
      if (strcmp(sName, fName) == 0)  break;
    if ((++idx) > fport->nfsearch) {
      struct _fsearch *newfs;
      newfs = realloc(fport->fsearchs,
                         ((idx + 1) * sizeof(struct _fsearch)));
      if (newfs == NULL) {
        puts("realloc failed: findport_results_get_for");
        return NULL;
      }
      fport->nfsearch++;
      fport->fsearchs = newfs;
      search = &newfs[idx];
      goto qfile_create;
    }
  } while (1);

  PhxObjectTextview *otxt = (PhxObjectTextview*)fport->param_data;
#if USE_MARKS
  PhxMark *search_mark = text_mark_get_type(otxt, PHXSEARCH);
  if (search_mark->dirtyo < search->sdirtyo)
    search->sdirtyo = search_mark->dirtyo;
  if (search->sdirtyo != INT_MAX) {
    findport_search_from(otxt, search, search->sdirtyo);
    findport_results_display(fport, search);
  }
#else
    // must update entire search, no access to dirtyo
  int rs = findport_search_from(otxt, search, 0);
  if (rs == 0)
    findport_results_reset(search);
#endif
  return search;
}

#if USE_MARKS
static void
_search_mark_update(PhxObjectTextview *otxt, void *data) {

    // currently, update part of findport_results_get_for()
  LCIFindPort *fport = (LCIFindPort*)data;
  struct _fsearch *search = findport_results_get_for(fport, NULL);
  if (search == NULL)  return;
}

static void
_search_mark_free(PhxObjectTextview *otxt) {

  PhxMark *search_mark = text_mark_get_type(otxt, PHXSEARCH);
  free(search_mark);
}

static void
_search_mark_set(PhxObjectTextview *otxt, LCIFindPort *fport) {
    // retreive mark, or create one
    // allows text change notify, and freeing of resourses on destroy
  PhxMark *search_mark = malloc(sizeof(PhxMark));
  search_mark->type = PHXSEARCH;
  search_mark->dirtyo = INT_MAX;
  search_mark->dirtyl = 0;
  search_mark->_mark_update = _search_mark_update;
  search_mark->_mark_free = _search_mark_free;
    // using LCIFindport, for multiple files, set data to NULL
  search_mark->data = fport;
  text_mark_list_add(otxt, search_mark);
}
#endif

/* This updates a replace of an entry */
    // only replace, replace & find, replace all
    // just update offsets instead of using strstr()
// given an entry, verify relaced entry alters, or deletes entry,
// if finds replacement contains sstring, reroutes to _search_from
static int
findport_update_entry(PhxObjectTextview *otxt, struct _fsearch *search,
                                                    int entry, int newSz) {

  struct _search_data *sdata = search->sdata;
  if (sdata->n_pairs == 0)  return 0;

  PhxMarkData *mdPtr = &search->sdata->pairs[entry];

  int start = mdPtr->d0;
  int delta = newSz - (mdPtr->d1 - start);
    // only replacing with a string longer than sstring
    // can a new sstring entry occur. replace with same not allowed
  if (delta > 0) {
    int end = (mdPtr + 1)->d0;
      // add in delta if not INT_MAX, textbuffer is altered, not mark
    if (end == INT_MAX)  end = otxt->str_nil;
    else end += delta;
    char *sPtr = strndup(&otxt->string[start], end - start);
    if (strstr(sPtr, search->sstring) != NULL) {
        // instead of new code, just pass this off to existing
      free(sPtr);
      return findport_search_from(otxt, search, start);
    }
    free(sPtr);
  }
    // now alter remaining by delta amount
  while ((mdPtr + 1)->d0 != INT_MAX) {
    mdPtr->d0 = (mdPtr + 1)->d0 + delta;
    mdPtr->d1 = (mdPtr + 1)->d1 + delta;
    mdPtr++;
  }
  mdPtr->d0 = INT_MAX;
  mdPtr->d1 = INT_MAX;
  return (--sdata->n_pairs);
}

/* Given an insert location, set search sdx based on directional request. */
static void
findport_move_to_mark(PhxObjectTextview *otxt,
                             struct _fsearch *search, int result_direction) {

  struct _search_data *sdata = search->sdata;
  if (sdata->n_pairs == 0)  return;

  int ins = otxt->insert.offset;
  PhxMarkData *mdPtr = sdata->pairs;
    // note: bsearch returns NULL if following condition
    // rather than adjust for NULL, quicker to test (+3 instructions to search)
  if (mdPtr->d0 > ins) {
    search->sdx = sdata->n_pairs - 1;
    if ((result_direction == RESULT_GE)
        || (result_direction == RESULT_GT)
        || (sdata->n_pairs == 1)) {
      search->sdx = 0;
    }
    return;
  }
  PhxMarkData *offset_mark = bsearch(&ins, mdPtr, sdata->n_pairs,
                              sizeof(PhxMarkData), _text_mark_offsets_compare);
  if (offset_mark->d0 == INT_MAX)  offset_mark--;
  search->sdx = offset_mark - mdPtr;
  if (ins == offset_mark->d0) {
    if ((result_direction == RESULT_GE)
        || (result_direction == RESULT_LE))
      return;
    if (result_direction == RESULT_LT) {
      if (search->sdx == 0)
            search->sdx = sdata->n_pairs - 1;
      else  search->sdx--;
      return;
    }
    if (result_direction == RESULT_GT) {
        // if not selected
      if (offset_mark->d1 != otxt->release.offset)
        return;
rgt:  if ((offset_mark + 1)->d0 != INT_MAX)
            search->sdx++;
      else  search->sdx = 0;
      return;
    }
  }
  if (result_direction == RESULT_GT)  goto rgt;
}

void
lci_findport_receiver_text(LCIFindPort *findPort, char ch,
                                                   char *data, size_t sz) {
  PhxObjectTextview *receiver;
  if (ch == 'c') {
    receiver = (PhxObjectTextview*)findPort->has_focus;
    if ((receiver != NULL) && (receiver->type == PHX_ENTRY))
      text_buffer_copy(receiver);
    return;
  }
  if (ch == 'x') {
    receiver = (PhxObjectTextview*)findPort->has_focus;
    if ((receiver != NULL) && (receiver->type == PHX_ENTRY)) {
      text_buffer_cut(receiver);
      cairo_region_t *crr;
      crr = cairo_region_create_rectangle(
               (cairo_rectangle_int_t *)&receiver->draw_box);
      gdk_window_invalidate_region(findPort->parent_window, crr, FALSE);
      cairo_region_destroy(crr);
    }
    return;
  }
  if (ch == 'v') {
    receiver = (PhxObjectTextview*)findPort->has_focus;
    if ((receiver != NULL) && (receiver->type == PHX_ENTRY)) {
      text_buffer_insert(receiver, data, sz);
      cairo_region_t *crr;
      crr = cairo_region_create_rectangle(
               (cairo_rectangle_int_t *)&receiver->draw_box);
      gdk_window_invalidate_region(findPort->parent_window, crr, FALSE);
      cairo_region_destroy(crr);
    }
  } else {
    int idx = (ch == 'e') ? textview_find_box : textview_replace_box;
    receiver = (PhxObjectTextview*)findPort->objects[idx];
    text_buffer_board_set(receiver, data, sz);
  }
}

/* demo/design/test has only 1 file, and no attribute set up as of yet */
/* demo also does not use gtk textbuffer ether */
void
lci_findport_search_delete(LCIFindPort *fport) {

  struct _fsearch *search = fport->fsearchs;
  if (search == NULL)  return;

  int sdx = 0;
  do {
    struct _fsearch *csPtr = &search[sdx];
    if (csPtr->sfile != NULL)
      free(csPtr->sfile);
    if (csPtr->sstring != NULL)
      free(csPtr->sstring);
    if (csPtr->sdata != NULL) {
      free(csPtr->sdata->pairs);
      free(csPtr->sdata);
    }
  } while ((++sdx) < fport->nfsearch);
  free(search);
  fport->nfsearch = 0;
  fport->fsearchs = NULL;
}

#pragma mark *** Findport Button Actions ***

void
lci_findport_search(LCIFindPort *fport) {

    // currently attaches only to single textview
  PhxObjectTextview *otxt = (PhxObjectTextview*)fport->param_data;
  if (otxt == NULL)  return;
    // verify something to find
  PhxObjectTextview *key_object
           = (PhxObjectTextview*)fport->objects[textview_find_box];
  if (key_object->str_nil == 0)  return;

    // first time will create empty slot, 'sfile' id == NULL
    // else returns 'sfile' id == NULL, for 'sstring'
  struct _fsearch *search = findport_results_get_for(fport, NULL);
  if (search == NULL)  return;

    // since lci_findport_search(), this is a 'new' search
    // clear tags and data
  if (search->sstring != NULL)
    findport_results_reset(search);
    // set what searching for
  search->sstring = strdup(key_object->string);

#if USE_MARKS
    // set textview edit change / destroy hook
    // note: without hook, each button action will trip full search
    // done via 'findport_results_get_for()'
  PhxMark *search_mark = text_mark_get_type(otxt, PHXSEARCH);
  if (search_mark == NULL)  _search_mark_set(otxt, fport);
#endif

    // search 'otxt' from offset '0', storing results in 'search'
    // use '0', lci_findport_search() considered first-time search of 'key'
  findport_search_from(otxt, search, 0);

    // fport has objects that change based on total found, sdata->n_pairs
  int found = findport_results_display(fport, search);
  if (found < 0)
    puts("failed error: findport_search_from");
  if (found <= 0)  return;

    // when search requested, could be from ^e or text entry with find
    // from insert offset, find first right to select
  findport_move_to_mark(otxt, search, RESULT_GE);
  text_buffer_apply_search_tag(otxt, search);
}

static void
findport_navigate_left(LCIFindPort *fport) {

  PhxObjectTextview *otxt = (PhxObjectTextview*)fport->param_data;
  if (otxt == NULL)  return;

  struct _fsearch *search = findport_results_get_for(fport, NULL);
  if (search == NULL)  return;

  findport_move_to_mark(otxt, search, RESULT_LT);
  text_buffer_apply_search_tag(otxt, search);
}

static void
findport_navigate_right(LCIFindPort *fport) {

  PhxObjectTextview *otxt = (PhxObjectTextview*)fport->param_data;
  if (otxt == NULL)  return;

  struct _fsearch *search = findport_results_get_for(fport, NULL);
  if (search == NULL)  return;

  findport_move_to_mark(otxt, search, RESULT_GT);
  text_buffer_apply_search_tag(otxt, search);
}

/* assumes sdx invalid */
static void
findport_replace(LCIFindPort *fport, int mode) {

  PhxObjectTextview *otxt = (PhxObjectTextview*)fport->param_data;
  if (otxt == NULL)  return;

  struct _fsearch *search = findport_results_get_for(fport, NULL);
  if (search == NULL)  return;
    // verify not without replaceable data
  if (search->sdata == NULL)  return;
  if (search->sdata->n_pairs == 0)  return;

    // supposedly the selection of found string.
    // must be a selected found string for replace to assume
    // it wasn't an accidental press of button.
  int ins = otxt->insert.offset;
  PhxMarkData *mdPtr = search->sdata->pairs;
    // note: bsearch returns NULL if following condition
    // rather than adjust for NULL, quicker to test (+3 instructions to search)
  if (mdPtr->d0 > ins)  return;
  PhxMarkData *offset_mark = bsearch(&ins, mdPtr, search->sdata->n_pairs,
                              sizeof(PhxMarkData), _text_mark_offsets_compare);
  if (offset_mark->d0 == INT_MAX)  offset_mark--;

    // check for button pressed with an actual selection of 'find'
  if (ins != offset_mark->d0)  return;
  if (otxt->release.offset != offset_mark->d1)  return;
  PhxObjectTextview *find;
  find = (PhxObjectTextview*)fport->objects[textview_find_box];
  if (memcmp(&otxt->string[offset_mark->d0], find->string, find->str_nil) != 0)
    return;
    // no test on replace, can be empty string
  PhxObjectTextview *replace;
  replace = (PhxObjectTextview*)fport->objects[textview_replace_box];

    // set sdx for findport_update_entry()
  search->sdx = offset_mark - mdPtr;

   // no test for 1, since possible replacement may contain sstring
  if ( (mode == 0)
      || ((mode == 3) && (search->sdata->n_pairs == 1)) ) {
      // replace does not alter locations, nor set up for editing
    text_buffer_replace(otxt, replace->string, replace->str_nil);
    findport_update_entry(otxt, search, search->sdx, replace->str_nil);
    findport_results_display(fport, search);
      // adjusment of locations
    text_buffer_edit_set(otxt, (ins + replace->str_nil));
    goto queue_redraw;
  }

  if (mode == 1) {
      // replace does not alter locations, nor set up for editing
    text_buffer_replace(otxt, replace->string, replace->str_nil);
    findport_update_entry(otxt, search, search->sdx, replace->str_nil);
    findport_results_display(fport, search);
      // adjusment of locations
    if (search->sdata->n_pairs == 0) {
      text_buffer_edit_set(otxt, (ins + replace->str_nil));
    } else {
      findport_move_to_mark(otxt, search, RESULT_GT);
      text_buffer_apply_search_tag(otxt, search);
    }
    goto queue_redraw;
  }

    // replace_all mode 3, does not 'Find' in replacement
    // want difference for placement of insert mark after done
  int delta = replace->str_nil - (mdPtr->d1 - mdPtr->d0);
  int ndx = search->sdata->n_pairs;  // holds count, not idx
  offset_mark = mdPtr + (ndx - 1);   // -1 for idx
    // start from tail, preserves forward mark positions
  do {
    otxt->insert.offset = offset_mark->d0;
    otxt->release.offset = offset_mark->d1;
    text_buffer_replace(otxt, replace->string, replace->str_nil);
    if ((--ndx) == 0)  break;
      // sdx = pre-button press location
    if (ndx <= search->sdx)  ins += delta;
    offset_mark--;
  } while (1);

  findport_results_reset(search);
  findport_results_display(fport, search);
    // adjusment of locations
  text_buffer_edit_set(otxt, (ins + replace->str_nil));


  cairo_region_t *crr;
queue_redraw:
  crr = cairo_region_create_rectangle(
           (cairo_rectangle_int_t *)&otxt->draw_box);
  gdk_window_invalidate_region(otxt->iface->parent_window, crr, FALSE);
  cairo_region_destroy(crr);
}

#pragma mark *** Main ***

static _Bool
configure_event_txtwnd(PhxInterface *iface, GdkEvent *event, void *widget) {

  (void)widget;
  int w_delta = event->configure.width - iface->mete_box.w;
  int h_delta = event->configure.height - iface->mete_box.h;
  iface->mete_box.w = event->configure.width;
  iface->mete_box.h = event->configure.height;

  if ((w_delta != 0) || (h_delta != 0)) {

    PhxObject *obj = iface->objects[0];
    ui_box_inset(&obj->mete_box, 0, 0, -w_delta, -h_delta);
    ui_box_inset(&obj->draw_box, 0, 0, -w_delta, -h_delta);
    obj = iface->objects[1];
    ui_box_inset(&obj->mete_box, 0, 0, -w_delta, -h_delta);
    ui_box_inset(&obj->draw_box, 0, 0, -w_delta, -h_delta);
    ui_box_inset(&((PhxObjectTextview*)obj)->bin, 0, 0, -w_delta, -h_delta);

    ui_interface_refresh(iface);
  }
  return FALSE;
}

static _Bool
configure_event_wnd(PhxInterface *fport,
                    GdkEvent *event, void *widget) {
  (void)widget;
  int width = event->configure.width;
    // silliness of gdk start up?
  if (width <= 1)  return FALSE;
  int w_delta = width - fport->mete_box.w;
  if (fport->mete_box.w != width) {
      // base object 'window' size change
    PhxObjectDrawing *odrw
                         = (PhxObjectDrawing*)fport->objects[0];
    ui_box_inset(&odrw->mete_box, 0, 0, -w_delta, 0);
    ui_box_inset(&odrw->draw_box, 0, 0, -w_delta, 0);
      // alter textview variables, move 'Done' x postion
      // since offseting, change right to opposite left
    PhxObjectButton *obtn = (PhxObjectButton*)fport->objects[close0_box];
    ui_box_inset(&obtn->mete_box, w_delta, 0, -w_delta, 0);
    ui_box_inset(&obtn->child->mete_box, w_delta, 0, -w_delta, 0);
    obtn = (PhxObjectButton*)fport->objects[close1_box];
    ui_box_inset(&obtn->mete_box, w_delta, 0, -w_delta, 0);
    ui_box_inset(&obtn->child->mete_box, w_delta, 0, -w_delta, 0);

    PhxObjectTextview *otxt
                = (PhxObjectTextview*)fport->objects[textview_find_box];
    ui_box_inset(&otxt->mete_box, 0, 0, -w_delta, 0);
    ui_box_inset(&otxt->draw_box, 0, 0, -w_delta, 0);
    ui_box_inset(&otxt->bin, 0, 0, -w_delta, 0);

    otxt = (PhxObjectTextview*)fport->objects[textview_replace_box];
    ui_box_inset(&otxt->mete_box, 0, 0, -w_delta, 0);
    ui_box_inset(&otxt->draw_box, 0, 0, -w_delta, 0);
    ui_box_inset(&otxt->bin, 0, 0, -w_delta, 0);
  }
  fport->mete_box.w = width;

  int height = event->configure.height;
  int h_delta = height - fport->mete_box.h;
  if (fport->mete_box.h != height) {
    PhxObjectDrawing *odrw
                         = (PhxObjectDrawing*)fport->objects[0];
    ui_box_inset(&odrw->mete_box, 0, 0, 0, h_delta);
    ui_box_inset(&odrw->draw_box, 0, 0, 0, h_delta);
  }
  fport->mete_box.h = height;

  if ( (w_delta != 0) || (h_delta != 0) )
    ui_interface_refresh(fport);

  return FALSE;
}

static void
btn_choose_event(PhxInterface *iface, GdkEvent *event, PhxObject *obj) {
    // reguardless of choice, both choices state 'Find'
  if (event->type != GDK_BUTTON_PRESS)  return;
  LCIFindPort *fport = (LCIFindPort*)iface;
  lci_findport_search(fport);
  ui_bank_combo_run(iface, event, obj);
}

static void
btn_replace_all_event(PhxInterface *iface, GdkEvent *event, PhxObject *obj) {
  if (event->type != GDK_BUTTON_PRESS)  return;
  LCIFindPort *fport = (LCIFindPort*)iface;
  findport_replace(fport, 3);
}

static void
btn_replace_event(PhxInterface *iface, GdkEvent *event, PhxObject *obj) {
  if (event->type != GDK_BUTTON_PRESS)  return;
  LCIFindPort *fport = (LCIFindPort*)iface;
  findport_replace(fport, 0);
}

static void
btn_replace_find_event(PhxInterface *iface, GdkEvent *event, PhxObject *obj) {
  if (event->type != GDK_BUTTON_PRESS)  return;
  LCIFindPort *fport = (LCIFindPort*)iface;
  findport_replace(fport, 1);
}

static void
btn_close_event(PhxInterface *iface, GdkEvent *event, PhxObject *obj) {
  if (event->type != GDK_BUTTON_RELEASE)  return;
  LCIFindPort *fport = (LCIFindPort*)iface;
  lci_findport_search_delete(fport);
  gtk_widget_hide(main_window);
  visible_set((PhxObject*)iface, FALSE);
}

static void
btn_navigate_right_event(PhxInterface *iface, GdkEvent *event, PhxObject *obj) {
  if (event->type != GDK_BUTTON_PRESS)  return;
  LCIFindPort *fport = (LCIFindPort*)iface;
  findport_navigate_right(fport);
}

static void
btn_navigate_left_event(PhxInterface *iface, GdkEvent *event, PhxObject *obj) {
  if (event->type != GDK_BUTTON_PRESS)  return;
  LCIFindPort *fport = (LCIFindPort*)iface;
  findport_navigate_left(fport);
}

static void
fw_initialize(PhxInterface *fport) {

  PhxObjectButton   *obtn;
  PhxObjectTextview *otxt;
  PhxObjectLabel    *olbl;

  int xpos = 0;
  int box_height = BOX_HEIGHT;

  int mdx = (box_height < 21) ? 0 : 1;
  int bbm = window_adjustments[mdx][2];

                        /* Combo Button [0,0] */
  obtn = (PhxObjectButton*)ui_object_create(fport,
                                     PHX_BUTTON_COMBO, draw_button,
                                     xpos, 0, fport->mete_box.w, box_height);
  ui_box_inset(&obtn->draw_box, bbm, bbm, bbm, bbm);
  ui_button_label_create(fport, obtn, "Find", HJST_LFT);
  int wcm2d = obtn->draw_box.w - obtn->child->mete_box.w;
  int wd2m = obtn->mete_box.w - obtn->draw_box.w;
    // add left margin
  obtn->child->draw_box.x += BUTTON_TEXT_MIN;
    // add right margin
  obtn->child->mete_box.w =
        obtn->child->draw_box.x + obtn->child->draw_box.w + BUTTON_TEXT_MIN;
    // label now has position (mete) and locations within (draw)
    // configure parent to match size changes
    // since mete.xyh stationary, alters w by reduced size
  obtn->draw_box.w = obtn->child->mete_box.w + wcm2d;
  obtn->mete_box.w = obtn->draw_box.w + wd2m;
    // assign activated cb
  obtn->_event_cb = btn_choose_event;
    // create returns the new obtn, original obtn now bank property
  obtn = ui_bank_create(PHX_BANK_COMBO, obtn, result_cb);
  PhxBank *obank = obtn->bank;

  obtn = (PhxObjectButton*)ui_object_create(fport,
                                     PHX_BUTTON_LABELED, draw_button,
                                     xpos, 0, fport->mete_box.w, box_height);
  ui_box_inset(&obtn->draw_box, bbm, bbm, bbm, bbm);
/* NOTE: All buttons designed around "Find & Replace" being used and longest */
  ui_button_label_create(fport, obtn, "Find & Replace", HJST_LFT);
  wcm2d = obtn->draw_box.w - obtn->child->mete_box.w;
  wd2m = obtn->mete_box.w - obtn->draw_box.w;
  obtn->child->draw_box.x += BUTTON_TEXT_MIN;
  obtn->child->mete_box.w =
        obtn->child->draw_box.x + obtn->child->draw_box.w + BUTTON_TEXT_MIN;
  obtn->draw_box.w = obtn->child->mete_box.w + wcm2d;
  obtn->mete_box.w = obtn->draw_box.w + wd2m;
  ui_bank_row_append(obank, obtn, TRUE);
    // used below, reset to match, plus need full size after appended
  obtn = obank->actuator;
  ui_interface_map(fport, (PhxObject*)obtn);

// want decrease in between button drawing, vertical alter mete location
// problem later if create mete_box clip
  int alter_y = bbm + 1;
  int alter_x = bbm;

                        /* Simple Button [1,0] */
    // want this button size same as the combo button's, consider as max,min
  int combo_width = obtn->mete_box.w;
  obtn = (PhxObjectButton*)ui_object_create(fport,
                        PHX_BUTTON_LABELED, draw_button,
                        xpos, (box_height - alter_y), combo_width, box_height);
  ui_box_inset(&obtn->draw_box, bbm, bbm, bbm, bbm);
  ui_button_label_create(fport, obtn, "Replace All", HJST_CTR);
  obtn->child->draw_box.x += BUTTON_TEXT_MIN;
  obtn->_event_cb = btn_replace_all_event;
  ui_interface_map(fport, (PhxObject*)obtn);
  visible_set((PhxObject*)obtn, FALSE);

                        /* Simple Button [1,1] */
  xpos += obtn->mete_box.w - alter_x;
    // using 'combo_width' as starting size, the max
  obtn = (PhxObjectButton*)ui_object_create(fport,
                        PHX_BUTTON_LABELED, draw_button,
                        xpos, (box_height - alter_y), combo_width, box_height);
  ui_box_inset(&obtn->draw_box, bbm, bbm, bbm, bbm);
  ui_button_label_create(fport, obtn, "Replace", HJST_CTR);
    // designed right spacing
  wcm2d = obtn->draw_box.w - obtn->child->mete_box.w;
  wd2m = obtn->mete_box.w - obtn->draw_box.w;
  obtn->child->draw_box.x += BUTTON_TEXT_MIN;
  obtn->child->mete_box.w =
        obtn->child->draw_box.x + obtn->child->draw_box.w + BUTTON_TEXT_MIN;
  obtn->draw_box.w = obtn->child->mete_box.w + wcm2d;
  obtn->mete_box.w = obtn->draw_box.w + wd2m;
  obtn->_event_cb = btn_replace_event;
  ui_interface_map(fport, (PhxObject*)obtn);
  visible_set((PhxObject*)obtn, FALSE);

                        /* Simple Button [1,2] */
  xpos += obtn->mete_box.w - alter_x;
    // this button same size as column 0
  obtn = (PhxObjectButton*)ui_object_create(fport,
                        PHX_BUTTON_LABELED, draw_button,
                        xpos, (box_height - alter_y), combo_width, box_height);
  ui_box_inset(&obtn->draw_box, bbm, bbm, bbm, bbm);
  ui_button_label_create(fport, obtn, "Replace & Find", HJST_CTR);
  obtn->child->draw_box.x += BUTTON_TEXT_MIN;
  obtn->_event_cb = btn_replace_find_event;
  ui_interface_map(fport, (PhxObject*)obtn);
  visible_set((PhxObject*)obtn, FALSE);

                        /* Simple Button [1,4] */
    // last in row, right justified
    // xpos: keep for textview start
  xpos += obtn->mete_box.w;
    // this starts at fport width ends at -close width
  obtn = (PhxObjectButton*)ui_object_create(fport,
                     PHX_BUTTON_LABELED, draw_button,
                     (fport->mete_box.w - combo_width), (box_height - alter_y),
                     combo_width, box_height);
  ui_box_inset(&obtn->draw_box, bbm, bbm, bbm, bbm);
  ui_button_label_create(fport, obtn, "Done", HJST_CTR);
    // this alters left instead of normal right, right justify button
  int delta = obtn->child->mete_box.w;
  wd2m = obtn->mete_box.x - obtn->draw_box.x;
  obtn->child->draw_box.x += BUTTON_TEXT_MIN;
  obtn->child->mete_box.w =
        obtn->child->draw_box.x + obtn->child->draw_box.w + BUTTON_TEXT_MIN;
    // this alters left instead of normal right, right justify button
  delta -= obtn->child->mete_box.w;
  obtn->child->mete_box.x += delta;
  obtn->draw_box.w -= delta;
  obtn->mete_box.w -= delta;
  obtn->mete_box.x += delta;
  obtn->_event_cb = btn_close_event;
  ui_interface_map(fport, (PhxObject*)obtn);
  visible_set((PhxObject*)obtn, FALSE);

                        /* Simple Button [0,4] */
  int done_width = obtn->mete_box.w;
  obtn = (PhxObjectButton*)ui_object_create(fport,
                     PHX_BUTTON_LABELED, draw_button,
                  (fport->mete_box.w - done_width), 0, done_width, box_height);
  ui_box_inset(&obtn->draw_box, bbm, bbm, bbm, bbm);
  ui_button_label_create(fport, obtn, "Done", HJST_CTR);
  obtn->_event_cb = btn_close_event;
  ui_interface_map(fport, (PhxObject*)obtn);

                        /* Textview [1,3] */
  otxt = (PhxObjectTextview*)ui_object_create(fport, PHX_ENTRY, draw_textview,
                         xpos, box_height - 1,
                         obtn->mete_box.x - xpos, (box_height - (2 * bbm)));
  ui_box_inset(&otxt->draw_box, 1, 1, 1, 1);
  ui_textview_font_set(otxt, otxt->draw_box.h);

  ui_textview_buffer_set(otxt, "replacing_text");
  ui_interface_map(fport, (PhxObject*)otxt);
  visible_set((PhxObject*)otxt, FALSE);

                        /* Textview [0,3] */
  otxt = (PhxObjectTextview*)ui_object_create(fport, PHX_ENTRY, draw_textview,
                         xpos, bbm,
                         obtn->mete_box.x - xpos, (box_height - (2 * bbm)));
  ui_box_inset(&otxt->draw_box, 1, 1, 1, 1);
  ui_textview_font_set(otxt, otxt->draw_box.h);
  ui_textview_buffer_set(otxt, "searched_text");
  ui_interface_map(fport, (PhxObject*)otxt);

                        /* Navigation Button [0,2] */
                        /* Navigation Button [0,2.5] */
    // should be 2 square buttons joined, child is drawing instead of label
  xpos = otxt->mete_box.x;
  obtn = (PhxObjectButton*)ui_object_create(fport,
                                             PHX_NAVIGATE_RIGHT, draw_navigate,
                                             (xpos - (box_height - bbm)), 0,
                                             (box_height - bbm), box_height);
  ui_box_inset(&obtn->draw_box, 0, bbm, bbm, bbm);
  obtn->_event_cb = btn_navigate_right_event;
  ui_interface_map(fport, (PhxObject*)obtn);
  sensitive_set((PhxObject*)obtn, FALSE);
  xpos = obtn->mete_box.x;
  obtn = (PhxObjectButton*)ui_object_create(fport,
                                             PHX_NAVIGATE_LEFT, draw_navigate,
                                             (xpos - (box_height - bbm)), 0,
                                             (box_height - bbm), box_height);
  ui_box_inset(&obtn->draw_box, bbm, bbm, 0, bbm);
  sensitive_set((PhxObject*)obtn, FALSE);
  obtn->_event_cb = btn_navigate_left_event;
  ui_interface_map(fport, (PhxObject*)obtn);

                        /* Label [0,1] */
  xpos = obtn->mete_box.x;
  int cbw = fport->objects[choose_box]->mete_box.w
           + fport->objects[choose_box]->mete_box.x;
  olbl = (PhxObjectLabel*)ui_object_create(fport, PHX_LABEL, draw_label,
                                           cbw, 0, (xpos - cbw), box_height);
  ui_box_inset(&olbl->draw_box, bbm, box_height/3.5, bbm, bbm);
  ui_label_set(olbl, "0 found matches", HJST_RGT);
  ui_interface_map(fport, (PhxObject*)olbl);
}

static void
tw_initialize(PhxInterface *iface) {

  PhxObjectTextview *otxt;

    // single object create
  char *buffer;
  long filesize;
  FILE *rh = fopen("find_bar.c", "r");
  if (rh == NULL) {  puts("file not found"); return;  }
  fseek(rh, 0 , SEEK_END);
  filesize = ftell(rh);
  fseek(rh, 0 , SEEK_SET);
  size_t rdSz = (filesize + TGAP_ALLOC) & ~(TGAP_ALLOC - 1);
  buffer = malloc(rdSz);
  memset(&buffer[filesize], 0, (rdSz - filesize));
  fread(buffer, filesize, 1, rh);
  fclose(rh);

  int box_width = iface->mete_box.w;
  int box_height = iface->mete_box.h;

  otxt = (PhxObjectTextview*)ui_object_create(iface, PHX_TEXTVIEW,
                                draw_textview, 0, 0, box_width, box_height);
    // Special Note: setting margins here, instead of after
    // ui_interface_map(), will omit margin areas from events.
  ui_box_inset(&otxt->draw_box, 1, 1, 1, 1);
  ui_textview_font_set(otxt, 16);
  ui_textview_buffer_set(otxt, buffer);
  ui_interface_map(iface, (PhxObject*)otxt);

  free(buffer);
}

static _Bool
tw_key_press_event(PhxInterface *iface, GdkEvent *event, GtkWidget *widget) {

  PhxObjectTextview *tv = (PhxObjectTextview*)iface->objects[1];

  if (!!(event->key.state & GDK_CONTROL_MASK)) {
    if ( (event->key.keyval == GDK_KEY_e)
        || (event->key.keyval == GDK_KEY_E) ) {
      if (findPort != NULL) {
        int ch = event->key.keyval;
        size_t sz = tv->release.offset - tv->insert.offset;
        if (sz != 0) {
          char *data = &tv->string[tv->insert.offset];
          lci_findport_receiver_text(findPort, ch, data, sz);
          if (event->key.keyval == GDK_KEY_e)
            if ( (main_window != NULL)
                && (gtk_widget_is_visible(main_window)) )
              lci_findport_search(findPort);
        }
      }
      return TRUE;
    }
    if (event->key.keyval == GDK_KEY_f) {
      if ( (main_window != NULL) && (!gtk_widget_is_visible(main_window)) ) {
        gtk_widget_show_all(main_window);
        visible_set((PhxObject*)findPort, TRUE);
      }
      lci_findport_search(findPort);
      return TRUE;
    }
  }
  return FALSE;
}

static _Bool
close_window(PhxInterface *iface, GdkEvent *event, GtkWidget *widget) {
  lci_findport_search_delete(findPort);
  gtk_widget_hide(widget);
  visible_set((PhxObject*)iface, FALSE);
  return TRUE;
}

int
main(int argc, char *argv[]) {

  PhxInterface      *tport;

  gtk_init(&argc, &argv); // initialize Gtk

  GdkDisplay *display = gdk_display_get_default();
  clipboard
        = gtk_clipboard_get_for_display(display, GDK_SELECTION_CLIPBOARD);

/* Need textview of file for testing */
  GtkWidget *text_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size(GTK_WINDOW(text_window), 800, 200);
  gtk_widget_realize(text_window);
    // signals for GTK_WINDOW_TOPLEVEL
  g_signal_connect(G_OBJECT(text_window), "destroy",
                                   G_CALLBACK(gtk_main_quit), NULL);

/* start of GTK_WINDOW_TOPLEVEL's interface */
  GtkWidget *textview = gtk_drawing_area_new();
  gtk_container_add(GTK_CONTAINER(text_window), textview);
  gtk_widget_realize(textview);

  tport = ui_interface_create((GtkDrawingArea*)textview, 0, 0, 800, 200);
  tw_initialize(tport);
    /* placed outside initialize to remind of need */
  tport->_configure_event = configure_event_txtwnd;

    // signals for GTK_WINDOW_TOPLEVEL, global 'commands'
    // place here to connect 'tport' as data
  g_signal_connect_swapped(G_OBJECT(text_window), "key-press-event",
                                G_CALLBACK(tw_key_press_event), tport);

  gtk_widget_show_all(text_window);

/* now interface. Note: main_window for demo showing
   normally only create 'drawing_area' and attach to application. */
  if (BOX_HEIGHT < 14) {
    puts("Can not honor height request < 14.");
    return 0;
  }
  int min_width = (int)((double)BOX_HEIGHT / .0406);
  int window_width = 600;
  if (window_width < min_width)
    puts("Forced to run as a dialog window due to requested width.");

    // main viewport
  int idx = (BOX_HEIGHT < 21) ? 0 : 1;
  int window_height = (BOX_HEIGHT * 2);
    // adjust view area to for margin differences
  int two_row_height = window_height + window_adjustments[idx][1];
  main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size(GTK_WINDOW(main_window),
                                            window_width, two_row_height);
  gtk_window_set_resizable(GTK_WINDOW(main_window), TRUE);
    // get attched to gdk
  gtk_widget_realize(main_window);

/* actual start of interface */
  GtkWidget *find_window = gtk_drawing_area_new();
  gtk_widget_set_size_request(find_window, min_width, BOX_HEIGHT);
  gtk_container_add(GTK_CONTAINER(main_window), find_window);

    // get attched to gdk
  gtk_widget_realize(find_window);
// This Interface expected to attach to global Application data
// For demo create a global variable
// For application, store variable in reserve slot
  findPort = (LCIFindPort*)ui_interface_create((GtkDrawingArea*)find_window,
                                        0, 0, window_width, two_row_height);
    // demo has only 1 text file attached
  findPort->param_data = tport->objects[1];
  visible_set((PhxObject*)findPort, FALSE);
  fw_initialize((PhxInterface*)findPort);
    /* placed outside initialize to remind of need */
  findPort->_configure_event = configure_event_wnd;

    // signals for GTK_WINDOW_TOPLEVEL
    // place here to connect 'findPort' as data
  g_signal_connect_swapped(G_OBJECT(main_window), "delete-event",
                                G_CALLBACK(close_window), findPort);
    // after full drawing of find window, only 'Find' is set visiable
    // resize to match
  gtk_window_resize(GTK_WINDOW(main_window), window_width, BOX_HEIGHT);


  gtk_main();

//  free(otxt->mark_list);
  free(findPort);
  free(tport);
  return EXIT_SUCCESS;
}
