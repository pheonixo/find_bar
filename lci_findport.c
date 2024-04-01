#include "lci_findport.h"
#include "phxobjects.c"

#pragma mark *** FindPort ***

// perform strstr() with u8 count returned for gtk's offset(glyph count)
// thus normal return pointer - s1 offset count may differ from returned gc
#ifndef __LITTLE_ENDIAN__
#define __LITTLE_ENDIAN__
#endif

#ifdef __LITTLE_ENDIAN__
 #define ALIGNSHIFT      <<
#else
 #define ALIGNSHIFT      >>
#endif

static char *
u8strstr(const char *s1, const char *s2, size_t *gc) {
    // determine strlen of key (s2), find nil byte
    // check first byte of s1 for match, but also test for 0xC0
    // 0xC0 is test for multibyte utf8 glyph
  uint32_t  t1, r0, r1;
  size_t a;
  unsigned char f = *(unsigned char *)s2;
  unsigned char *dst = (unsigned char *)s2;
  if (!f)  return (char *)s1;
  r0 = 0x01010101;
  r1 = 0x80808080;
  a = (size_t)dst & (size_t)3;
  t1 = (*(uint32_t *)(dst -= a)) | ~((uint32_t)0xffffffff ALIGNSHIFT (a << 3));
  while (!((t1 - r0) & (~t1 & r1)))  dst += 4,  t1 = *(uint32_t *)dst;

#ifdef __LITTLE_ENDIAN__
  if (!(t1 & 0x000000ff))  goto proceed;  dst++;
  if (!(t1 & 0x0000ff00))  goto proceed;  dst++;
  if (!(t1 & 0x00ff0000))  goto proceed;  dst++;
#else
  if (!(t1 & 0xff000000))  goto proceed;  dst++;
  if (!(t1 & 0x00ff0000))  goto proceed;  dst++;
  if (!(t1 & 0x0000ff00))  goto proceed;  dst++;
#endif
proceed:
  {
    size_t n, m, g;
    unsigned char *str, c;
    n = (m = (size_t)dst - (size_t)s2);
    str = (unsigned char *)s1;
    g = 0;
    while ((c = *str++)) {
      if ((c & 0x0C0) != 0x080) g++;
      if (c == f) {
        unsigned char ch, *find = (unsigned char *)s2;
        unsigned char *pStr = str;
        do {  ch = *++find;  if (!(--n)) goto str_exit;  } while (ch == *pStr++);
        n = m;
      }
    }
    return NULL;
  str_exit:
    *gc = --g;
    return (char *)--str;
  }
}

/* 
    From a start pointer(s1), count the number of utf8 character codes
   in (n) bytes.
    Will return codes found, but will be a negative should (n) not
   complete a code. Returns 0 if (n) is negative or 0.
    Does not validate codes.
*/
static ssize_t
u8count(const char *s1, ssize_t n) {

 ssize_t count = 0;
 if (n > 0) {
   do {
     if ((*s1 & 0x0C0) != 0x080) count++;
     s1++;
   } while ((--n));
   if ((*s1 & 0x0C0) == 0x080) {
     count--;
     count = -count;
   }
 }
 return count;
}

// given a number of valid utf8 glyph codes starting at (s1), return length
static size_t
u8offset(const char *s1, ssize_t codes) {

 size_t count = 0;
 if (codes > 0) {
   do {
     if ((*s1 & 0x0C0) == 0x0C0)
       while ((s1[1] & 0x0C0) == 0x080)  count++, s1++;
     count++, s1++;
   } while ((--codes));
 }
 return count;
}


static void
result_cb(PhxBank *obank) {

  PhxObjectButton *obtn = (PhxObjectButton*)obank->actuator;
  LCIEditorArea *ea = ((LCIFindPort*)obtn->iface)->session->editor_area;
  LCIFindPort *findPort = ea->findport;
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
  GtkWidget *find_window = ea->find_window;
  if (set) {
    if (findPort->mete_box.h != two_row_height)
      gtk_widget_set_size_request(find_window, min_width, two_row_height);
  } else {
    if (findPort->mete_box.h == two_row_height)
      gtk_widget_set_size_request(find_window, min_width, BOX_HEIGHT);
  }
}

/*
    Given an insert/release pair offset, make it highlit and scroll
   it into view.
    Special code must be added to convert offsets to glyph counts
   for gtk use.
*/
static void
text_buffer_apply_search_tag(LCITextPort *active, struct _fsearch *search) {

  if (search->sdata->n_pairs == 0)  return;
  PhxMarkData *mdPtr = &search->sdata->pairs[search->sdx];

  GtkTextBuffer *tbuf = GTK_TEXT_BUFFER(active->textbuffer);

  GtkTextIter begin, end;
  gtk_text_buffer_get_start_iter(tbuf, &begin);
  gtk_text_buffer_get_end_iter(tbuf, &end);
  char *sPtr = gtk_text_buffer_get_text(tbuf, (const GtkTextIter*)&begin,
                                              (const GtkTextIter*)&end, FALSE);

    // NOTE: convert offset to glyph count for
    // gtk_text_buffer_get_iter_at_glyph
  ssize_t cnt0 = u8count((const char *)sPtr, mdPtr->d0);
  ssize_t cnt1 = u8count((const char *)(sPtr + mdPtr->d0),
                                       (mdPtr->d1 - mdPtr->d0));
    // gtk expects glyph counts not offsets
  gtk_text_buffer_get_iter_at_offset(tbuf, &begin, cnt0);
  gtk_text_buffer_get_iter_at_offset(tbuf, &end, (cnt0 + cnt1));
  gtk_text_buffer_select_range(tbuf, (const GtkTextIter*)&begin,
                                     (const GtkTextIter*)&end);
  gtk_text_view_scroll_to_iter(
              GTK_TEXT_VIEW(active->textview), &end, 0.3, TRUE, 0.5, 0.5);
  g_free(sPtr);
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
    LCIEditorArea *ea = fport->session->editor_area;
    LCITextPort *active = ea->editor_view[0];
    GtkTextBuffer *tbuf = GTK_TEXT_BUFFER(active->textbuffer);

    GtkTextIter start, end;
    gtk_text_buffer_get_selection_bounds(tbuf, &start, &end);
    int ins = gtk_text_iter_get_offset((const GtkTextIter*)&start);
    int rel = gtk_text_iter_get_offset((const GtkTextIter*)&end);
    set = ((ins != fdata->sdata->pairs->d0)
           || (rel != fdata->sdata->pairs->d1));
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
findport_search_from(GtkTextBuffer *tbuf,
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

    // get tag, create or existing for "search_result"
  GtkTextTag *tag;
  GtkTextIter begin, end;
  GtkTextTagTable *ttab = gtk_text_buffer_get_tag_table(tbuf);
  if ((tag = gtk_text_tag_table_lookup(ttab, "search_result")) == NULL)
    tag = gtk_text_buffer_create_tag(tbuf, "search_result",
                                   "background", "rgba(255,215,0,0.5)", NULL);

  gtk_text_buffer_get_start_iter(tbuf, &begin);
  GtkTextIter new = begin;
  gtk_text_iter_set_offset(&new, start);
  gtk_text_buffer_get_end_iter(tbuf, &end);
    // clear tags, then result data
  gtk_text_buffer_remove_tag(tbuf, tag, (const GtkTextIter*)&new,
                                        (const GtkTextIter*)&end);

  gtk_text_buffer_get_end_iter(tbuf, &end);
  char *sPtr = gtk_text_buffer_get_text(tbuf, (const GtkTextIter*)&begin,
                                              (const GtkTextIter*)&end, FALSE);
  char *rdPtr = sPtr + start;
    // need glyph count up to start
  size_t lgc = 0;
  if (start != 0)
    lgc = u8count(sPtr, start);

  size_t key_len = strlen(key);
  size_t kgc = 0;
  for (int i = 0; i < (int)key_len; i++)
    if ((key[i] & 0x0C0) != 0x080) kgc++;

  size_t gc;
  while ((rdPtr = u8strstr(rdPtr, key, &gc)) != NULL) {
    mdPtr->d0 = rdPtr - sPtr;
    mdPtr->d1 = mdPtr->d0 + key_len;
      // NOTE: gtk function is gtk_text_buffer_get_iter_at_glyph
      // need to convert offset to glyph count
    lgc += gc;
    gtk_text_buffer_get_iter_at_offset(tbuf, &begin, lgc);
    gtk_text_buffer_get_iter_at_offset(tbuf, &end, lgc + kgc);
    gtk_text_buffer_apply_tag(tbuf, tag, (const GtkTextIter*)&begin,
                                         (const GtkTextIter*)&end);
      // set start points after 'end'
    rdPtr += key_len;
    lgc += kgc;
    mdPtr++;
    if ((++ndx) == acheck) {
      mdPtr = _mark_pairs_realloc(&sdata->pairs, ndx);
      if (mdPtr == NULL)  return -1;
      acheck += MARK_ALLOC;
    }
  }
  mdPtr->d0 = INT_MAX;
  mdPtr->d1 = INT_MAX;
  g_free(sPtr);
  sdata->n_pairs = ndx;
    // return count
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

    // must update entire search, no access to dirtyo
  LCIEditorArea *ea = fport->session->editor_area;
  LCITextPort *active = ea->editor_view[0];
  GtkTextBuffer *tbuf = GTK_TEXT_BUFFER(active->textbuffer);
  int rs;
  rs = findport_search_from(tbuf, search, 0);
  if (rs == 0)
    findport_results_reset(search);
  findport_results_display(fport, search);
  return search;
}

/* This updates a replace of an entry */
    // only replace, replace & find, replace all
    // just update offsets instead of using strstr()
// given an entry, verify relaced entry alters, or deletes entry,
// if finds replacement contains sstring, reroutes to _search_from
// MUST: uses gtk offsets, so convert offsets to glyph counts
static int
findport_update_entry(GtkTextBuffer *tbuf, struct _fsearch *search,
                                                    int entry, int newSz) {

  struct _search_data *sdata = search->sdata;
  if (sdata->n_pairs == 0)  return 0;

  PhxMarkData *mdPtr = &search->sdata->pairs[entry];

  int start = mdPtr->d0;
  int delta = newSz - (mdPtr->d1 - start);
  mdPtr->d1 += delta;
    // only replacing with a string longer than sstring
    // can a new sstring entry occur. replace with same not allowed
  if (delta > 0) {
    char *sPtr;
    int end;
    GtkTextIter ts, te;
    gtk_text_buffer_get_start_iter(tbuf, &ts);
    gtk_text_buffer_get_end_iter(tbuf, &te);
    sPtr = gtk_text_buffer_get_text(tbuf, (const GtkTextIter*)&ts,
                                          (const GtkTextIter*)&te, FALSE);
    start = u8count(sPtr, start);
    g_free(sPtr);
    end = start + newSz;
    gtk_text_buffer_get_iter_at_offset(tbuf, &ts, start);
    if ((mdPtr + 1)->d0 == INT_MAX) {
      gtk_text_buffer_get_end_iter(tbuf, &te);
      end = gtk_text_iter_get_offset((const GtkTextIter*)&te);
    }
    gtk_text_buffer_get_iter_at_offset(tbuf, &te, end);

      // add in delta if not INT_MAX, textbuffer is altered, not mark 
    sPtr = gtk_text_buffer_get_text(tbuf, (const GtkTextIter*)&ts,
                                          (const GtkTextIter*)&te, FALSE);

    if (strstr(sPtr, search->sstring) != NULL) {
        // instead of new code, just pass this off to existing
      g_free(sPtr);
      return findport_search_from(tbuf, search, mdPtr->d0);
    }
    g_free(sPtr);
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

/*
    Given an insert location, set search sdx based on directional request.
    Special code must be added to convert offsets to glyph counts
   for gtk use.
*/
static void
findport_move_to_mark(GtkTextBuffer *tbuf,
                             struct _fsearch *search, int result_direction) {

  struct _search_data *sdata = search->sdata;
  if (sdata->n_pairs == 0)  return;

    // uses gtk_text_iter functions, convert glyph to offset
  GtkTextIter start, end;
  gtk_text_buffer_get_start_iter(tbuf, &start);
  gtk_text_buffer_get_end_iter(tbuf, &end);
  char *sPtr = gtk_text_buffer_get_text(tbuf, (const GtkTextIter*)&start,
                                              (const GtkTextIter*)&end, FALSE);

  gtk_text_buffer_get_selection_bounds(tbuf, &start, &end);
  int ins = gtk_text_iter_get_offset((const GtkTextIter*)&start);
  int rel = gtk_text_iter_get_offset((const GtkTextIter*)&end);

  rel -= ins;
  ins = u8offset(sPtr, ins);
  rel = u8offset(sPtr + ins, rel) + ins;
  g_free(sPtr);

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
      if (offset_mark->d1 != rel)
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

void
lci_findport_search_delete(LCIFindPort *fport) {

  struct _fsearch *search = fport->fsearchs;
  if (search == NULL)  return;

  LCIEditorArea *ea = fport->session->editor_area;
  int sdx = 0;
  do {
    int idx;
    struct _fsearch *csPtr = &search[sdx];
    char *sName = csPtr->sfile;
    for (idx = 1; idx < ea->n_view; idx++) {
      char *fName = ea->editor_view[idx]->filename;
      if ((sName == NULL) || (fName == NULL)) { if (sName == fName)  break; }
      else if (strcmp(sName, fName) == 0)  break;
    }
    GtkTextIter start, end;
    GtkTextBuffer *tbuf = GTK_TEXT_BUFFER(ea->editor_view[idx]->textbuffer);
    gtk_text_buffer_get_start_iter(tbuf, &start);
    gtk_text_buffer_get_end_iter(tbuf, &end);
    gtk_text_buffer_remove_tag_by_name(tbuf, "search_result",
                                              (const GtkTextIter*)&start,
                                              (const GtkTextIter*)&end);
    if (csPtr->sfile != NULL)
      free(csPtr->sfile);
    if (csPtr->sstring != NULL)
      free(csPtr->sstring);
    if (csPtr->sdata != NULL) {
      free(csPtr->sdata->pairs);
      free(csPtr->sdata);
    }
  } while ((++sdx) <= fport->nfsearch);
  free(search);
  fport->nfsearch = 0;
  fport->fsearchs = NULL;
}

#pragma mark *** Findport Button Actions ***

void
lci_findport_search(LCIFindPort *fport) {

  LCIEditorArea *ea = fport->session->editor_area;
  LCITextPort *active = ea->editor_view[0];
  if (active == NULL)  return;
    // only place to hook in with gtk
  visible_set((PhxObject*)fport, TRUE);

  PhxObjectTextview *key_object
           = (PhxObjectTextview*)fport->objects[textview_find_box];
  if (key_object->str_nil == 0)  return;

    // first time will create empty slot, 'sfile' id == NULL
    // else returns 'sfile' id == NULL, for 'sstring'
  struct _fsearch *search = findport_results_get_for(fport, active->filename);
  if (search == NULL)  return;

    // since lci_findport_search(), this is a 'new' search
    // clear tags and data
  if (search->sstring != NULL)
    findport_results_reset(search);
    // set what searching for
  search->sstring = strdup(key_object->string);

    // search 'otxt' from offset '0', storing results in 'search'
    // use '0', lci_findport_search() considered first-time search of 'key'
  GtkTextBuffer *tbuf = GTK_TEXT_BUFFER(active->textbuffer);
  findport_search_from(tbuf, search, 0);

    // fport has objects that change based on total found, sdata->n_pairs
  int found = findport_results_display(fport, search);
  if (found < 0)
    puts("failed error: findport_search_from");
  if (found <= 0)  return;

    // when search requested, could be from ^e or text entry with find
    // from insert offset, find first right to select
  findport_move_to_mark(tbuf, search, RESULT_GE);
  text_buffer_apply_search_tag(active, search);
}

static void
findport_navigate_left(LCIFindPort *fport) {

  LCIEditorArea *ea = fport->session->editor_area;
  LCITextPort *active = ea->editor_view[0];
  if (active == NULL)  return;

  struct _fsearch *search = findport_results_get_for(fport, active->filename);
  if (search == NULL)  return;

  GtkTextBuffer *tbuf = GTK_TEXT_BUFFER(active->textbuffer);
  findport_move_to_mark(tbuf, search, RESULT_LT);
  text_buffer_apply_search_tag(active, search);
}

static void
findport_navigate_right(LCIFindPort *fport) {

  LCIEditorArea *ea = fport->session->editor_area;
  LCITextPort *active = ea->editor_view[0];
  if (active == NULL)  return;

  struct _fsearch *search = findport_results_get_for(fport, active->filename);
  if (search == NULL)  return;

  GtkTextBuffer *tbuf = GTK_TEXT_BUFFER(active->textbuffer);
  findport_move_to_mark(tbuf, search, RESULT_GT);
  text_buffer_apply_search_tag(active, search);
}

/* assumes sdx invalid, conversion of glyph/offset verified */
static void
findport_replace(LCIFindPort *fport, int mode) {

  LCIEditorArea *ea = fport->session->editor_area;
  LCITextPort *active = ea->editor_view[0];
  if (active == NULL)  return;

  struct _fsearch *search = findport_results_get_for(fport, active->filename);
  if (search == NULL)  return;
    // verify not without replaceable data
  if (search->sdata == NULL)  return;
  if (search->sdata->n_pairs == 0)  return;

    // verify selection is actual 'search' data to be replaced
    // uses gtk_text_iter functions, convert glyph to offset
  GtkTextIter start, end;
  GtkTextBuffer *tbuf = GTK_TEXT_BUFFER(active->textbuffer);

  gtk_text_buffer_get_start_iter(tbuf, &start);
  gtk_text_buffer_get_end_iter(tbuf, &end);
  char *sPtr = gtk_text_buffer_get_text(tbuf, (const GtkTextIter*)&start,
                                              (const GtkTextIter*)&end, FALSE);

  if (gtk_text_buffer_get_selection_bounds(tbuf, &start, &end) == FALSE)
    return;
  int ins = gtk_text_iter_get_offset((const GtkTextIter*)&start);
  int rel = gtk_text_iter_get_offset((const GtkTextIter*)&end);

  rel -= ins;
  ins = u8offset(sPtr, ins);
  rel = u8offset(sPtr + ins, rel) + ins;

  PhxMarkData *mdPtr = search->sdata->pairs;
    // note: bsearch returns NULL if following condition
    // rather than adjust for NULL, quicker to test (+3 instructions to search)
  if (mdPtr->d0 > ins)  return;
  PhxMarkData *offset_mark = bsearch(&ins, mdPtr, search->sdata->n_pairs,
                              sizeof(PhxMarkData), _text_mark_offsets_compare);
  if (offset_mark->d0 == INT_MAX)  offset_mark--;

    // check for button pressed with an actual selection of 'find'
  if (ins != offset_mark->d0)  return;
  if (rel != offset_mark->d1)  return;

  PhxObjectTextview *replace;
  replace = (PhxObjectTextview*)fport->objects[textview_replace_box];

  char *selected = gtk_text_buffer_get_text(tbuf,
                                          (const GtkTextIter*)&start,
                                          (const GtkTextIter*)&end, FALSE);
  _Bool same = (memcmp(selected, replace->string, replace->str_nil) == 0);
  g_free(selected);

  if ( ((offset_mark->d1 - offset_mark->d0) == replace->str_nil) && same ) {
    g_free(sPtr);
    return;
  }

    // passed verifaction, now replace based on button action requested

    // set sdx for findport_update_entry()
  search->sdx = offset_mark - mdPtr;

   // no test for 1, since possible replacement may contain sstring


  if ( !((mode == 3) && (search->sdata->n_pairs > 1)) ) {

    gtk_text_buffer_begin_user_action(tbuf);
    gtk_text_buffer_delete(tbuf, &start, &end);
    gtk_text_buffer_insert(tbuf, &start, replace->string, replace->str_nil);
    gtk_text_buffer_end_user_action(tbuf);
    g_free(sPtr);

    findport_update_entry(tbuf, search, search->sdx, replace->str_nil);
    findport_results_display(fport, search);

    if (mode == 1) {
        // adjusment of locations
      findport_move_to_mark(tbuf, search, RESULT_GT);
      text_buffer_apply_search_tag(active, search);
    }
    return;
  }

    // replace_all mode 3, does not 'Find' in replacement
    // start from tail, preserves forward mark positions
  offset_mark = mdPtr + (search->sdata->n_pairs - 1);
  int pos0 = u8count(sPtr, offset_mark->d0);
  int r0 = u8count(sPtr + offset_mark->d0, (offset_mark->d1 - offset_mark->d0));
  gtk_text_buffer_begin_user_action(tbuf);
  do {
      // unfortunate we start at tail, u8count from begining
      // but delay probly good for gtk
    gtk_text_buffer_get_iter_at_offset(tbuf, &start, pos0);
    gtk_text_buffer_get_iter_at_offset(tbuf, &end, (pos0 + r0));
    gtk_text_buffer_delete(tbuf, &start, &end);
    gtk_text_buffer_insert(tbuf, &start, replace->string, replace->str_nil);
    if ((--search->sdata->n_pairs) == 0)  break;
    pos0 = u8count(sPtr, (--offset_mark)->d0);
  } while (1);
  gtk_text_buffer_end_user_action(tbuf);
  g_free(sPtr);

  findport_results_reset(search);
  findport_results_display(fport, search);
}

#pragma mark *** Main ***

static _Bool
configure_event(PhxInterface *fport, GdkEvent *event, void *widget) {

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
  gtk_widget_hide(fport->session->editor_area->find_window);
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

/* Add a second box to 'editor_box' container.
 * Need container to be 'find_box' and editor_box'.
 * Keep 'editor_box' as refering to viewport container for
 * editor, used as that throughout code. Since GTK uses
 * box as sub-class of container, refer to old editor_box as
 * editor_container, which will house boxes as needed, but always
 * be container for 'editor_box'. */

GtkWidget *
lci_findport_create(LCISession *session) {

  if (BOX_HEIGHT < 14) {
    puts("Can not honor height request < 14.");
    return NULL;
  }

  LCIEditorArea *ea = session->editor_area;
  GtkWidget *eb = ea->editor_box;

    // use internal until figure out global set up
  GdkDisplay *display = gdk_display_get_default();
  clipboard
        = gtk_clipboard_get_for_display(display, GDK_SELECTION_CLIPBOARD);

  int window_height = BOX_HEIGHT;
  int min_width = (int)((double)BOX_HEIGHT / .0406);
  int window_width = gtk_widget_get_allocated_width(eb);
  if (window_width < min_width)
    puts("Forced to run as a dialog window due to requested width.");
  int idx = (BOX_HEIGHT < 21) ? 0 : 1;
  int two_row_height = (window_height * 2) + window_adjustments[idx][1];

  GtkWidget *find_window = gtk_drawing_area_new();
    // keep port, even when inactive, hidden
  g_object_ref(G_OBJECT(find_window));
  gtk_widget_set_size_request(find_window, min_width, window_height);
  gtk_widget_set_hexpand(find_window, TRUE);

    // NOTE: all textviews are packed at end
  gtk_box_pack_start(GTK_BOX(eb), find_window, FALSE, FALSE, 0);

    // get attched to gdk
  gtk_widget_realize(find_window);
    // create
  LCIFindPort *findPort
          = (LCIFindPort*)ui_interface_create((GtkDrawingArea*)find_window,
                                        0, 0, window_width, two_row_height);
  ea->findport = findPort;
  visible_set((PhxObject*)findPort, FALSE);
  ea->find_window = find_window;
  findPort->session = session;
  fw_initialize((PhxInterface*)findPort);
  findPort->_configure_event = configure_event;

  return find_window;
}

