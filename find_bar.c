#include "find_bar.h"

static void
visible_set(PhxObject *obj, _Bool visible) {
  if (visible) {
    obj->state &= ~(0x00000001U << (VISUAL + 0));
  } else {
    obj->state |= (0x00000001U << (VISUAL + 0));
  }
}

static __inline__ _Bool
visible_get(PhxObject *obj) {
  return ((obj->state & (0x00000001U << (VISUAL + 0))) == 0);
}

static void
sensitive_set(PhxObject *obj, _Bool sensitive) {
    // must be visiable
  if (visible_get(obj)) {
    if (sensitive)
      obj->state &= ~(0x00000001U << (VISUAL + 1));
    else
      obj->state |= (0x00000001U << (VISUAL + 1));
  }
}

static __inline__ _Bool
sensitive_get(PhxObject *obj) {
  return ((obj->state & (0x00000001U << (VISUAL + 1))) == 0);
}

#pragma mark *** TextMarks ***

#if USE_MARKS

static int
_text_marks_offsets_compare(const void *a, const void *b) {

  int key = *((int*)a);
  PhxMark *mAsk0 = (PhxMark*)b;
  int lineStart  = mAsk0->offset;
  if (lineStart == INT_MAX)  return -1;

  PhxMark *mAsk1 = mAsk0 + 1;
  int lineEnd    = mAsk1->offset;

  if (key < lineStart)  return -1;
  if ((key == lineStart) || (key < lineEnd))  return 0;
  return 1;
}

static int
_text_marks_lines_compare(const void *a, const void *b) {

  int key = *((int*)a);
  PhxMark *mAsk0 = (PhxMark*)b;
  if (mAsk0->offset == INT_MAX) {
    if (key >= mAsk0->line)  return 0;
    return -1;
  }
  int lineStart = mAsk0->line;

  PhxMark *mAsk1 = mAsk0 + 1;
  int lineEnd = (mAsk1->offset == INT_MAX) ? INT_MAX : mAsk1->line;

  if (key < lineStart)  return -1;
  if ((key == lineStart) || (key < lineEnd))  return 0;
  return 1;
}

static void
text_marks_initialize_for(PhxObjectTextview *otxt) {

  if (otxt->newline_list != NULL)
    free(otxt->newline_list);

    // this @ time typing this almost 2800 lines
  otxt->newline_list = malloc(MARK_ALLOC * sizeof(PhxMark));
  memset(otxt->newline_list, 0, (MARK_ALLOC * sizeof(PhxMark)));
  otxt->list_size  = MARK_ALLOC;
  otxt->list_ldelta = 0;
  otxt->list_dirtyo = INT_MAX;

  PhxMark *mPtr = otxt->newline_list;
  mPtr->type = PHXLINE;
  //mPtr->offset = 0;
  //mPtr->line = 0;
  otxt->list_nil = 1;

  if (otxt->string != NULL) {
    char *nPtr = otxt->string;
    do {
      nPtr = strchr(nPtr, '\n');
      if (nPtr == NULL)  break;
      (++mPtr)->type = PHXLINE;
      mPtr->offset = (++nPtr) - otxt->string;
      mPtr->line = otxt->list_nil;
      if ((++otxt->list_nil) == otxt->list_size) {
        size_t newSz = otxt->list_size + MARK_ALLOC;
        PhxMark *newPtr
                    = realloc(otxt->newline_list, (newSz * sizeof(PhxMark)));
        if (newPtr == NULL) {
          puts("realloc failure: text_marks_initialize_for");
          return;
        }
        otxt->newline_list = newPtr;
        otxt->list_size = newSz;
        mPtr = &otxt->newline_list[(otxt->list_nil - 1)];
      }
    } while (1);
  }
  (++mPtr)->type = PHXLINE;
  mPtr->offset = INT_MAX;
  mPtr->line = otxt->list_nil;
  if ((otxt->list_nil + 1) == otxt->list_size) {
    size_t newSz = otxt->list_size + MARK_ALLOC;
    PhxMark *newPtr = realloc(otxt->newline_list, (newSz * sizeof(PhxMark)));
    if (newPtr == NULL) {
      puts("realloc failure: text_marks_initialize_for");
      return;
    }
    otxt->newline_list = newPtr;
    otxt->list_size = newSz;
  }
}

static void
text_marks_DnD_copy_update(PhxObjectTextview *tv) {

  PhxMark *insert_mark, *release_mark, *drop_mark;

  drop_mark    = bsearch(&tv->drop.offset, tv->newline_list,
                                  tv->list_nil,
                                  sizeof(PhxMark),
                                  _text_marks_offsets_compare);
  insert_mark  = bsearch(&tv->insert.offset, tv->newline_list,
                                  tv->list_nil,
                                  sizeof(PhxMark),
                                  _text_marks_offsets_compare);
  release_mark = bsearch(&tv->release.offset, insert_mark,
                                  tv->list_nil - insert_mark->line,
                                  sizeof(PhxMark),
                                  _text_marks_offsets_compare);
      // add 1 for cushion for now
  int lines_affected = release_mark->line - insert_mark->line;
  if (lines_affected == 0) {
    int delta = tv->release.offset - tv->insert.offset;
    while ((++drop_mark)->offset != INT_MAX)
      drop_mark->offset += delta;
    return;
  }

  if ((tv->list_nil + lines_affected + 1) >= tv->list_size) {
    puts("realloc: text_marks_DnD_copy_update"); return;
  }
  memmove(drop_mark + lines_affected, drop_mark,
               (tv->list_nil + 1 - drop_mark->line) * sizeof(PhxMark));
  if (drop_mark < insert_mark) {
    insert_mark += lines_affected;
    release_mark += lines_affected;
  }

  int ndx = drop_mark->line;
  int delta = tv->drop.offset - drop_mark->offset;
  delta += (insert_mark + 1)->offset - tv->insert.offset;
  (drop_mark + 1)->offset = drop_mark->offset + delta;
  (++drop_mark)->line = (++ndx);

  while ((--lines_affected) != 0) {
    insert_mark++;
    delta = (insert_mark + 1)->offset - insert_mark->offset;
    (drop_mark + 1)->offset = drop_mark->offset + delta;
    (++drop_mark)->line = (++ndx);
  }

  delta = tv->release.offset - tv->insert.offset;
  while ((++drop_mark)->offset != INT_MAX) {
    drop_mark->offset += delta;
    drop_mark->line = (++ndx);
  }
  drop_mark->line = (tv->list_nil = (++ndx));
}

/* could possibly invert selections to decrease code size */
static void
text_marks_DnD_move_update(PhxObjectTextview *tv) {

  PhxMark *insert_mark, *release_mark, *drop_mark;

  if (tv->drop.offset < tv->insert.offset) {

    drop_mark    = bsearch(&tv->drop.offset, tv->newline_list,
                                  tv->list_nil,
                                  sizeof(PhxMark),
                                  _text_marks_offsets_compare);
    release_mark = bsearch(&tv->release.offset, drop_mark,
                                  tv->list_nil - drop_mark->line,
                                  sizeof(PhxMark),
                                  _text_marks_offsets_compare);
    insert_mark  = bsearch(&tv->insert.offset, drop_mark,
                                  release_mark->line + 1 - drop_mark->line,
                                  sizeof(PhxMark),
                                  _text_marks_offsets_compare);

    if (insert_mark == drop_mark) {
      if (release_mark != insert_mark) {
        int delta = tv->insert.offset - tv->drop.offset;
        while (release_mark > drop_mark) {
          release_mark->offset -= delta;
          release_mark--;
        }
      }
      return;
    }

      // add 1 for cushion for now
    int lines_affected = release_mark->line - insert_mark->line + 1;
    if ((tv->list_nil + lines_affected + 1) >= tv->list_size) {
      puts("realloc: text_marks_DnD_move_update"); return;
    }
      // copies, at min, the next line after insert_mark i=0+1 to i=0+1+la
    PhxMark *mInsert = &tv->newline_list[(tv->list_nil + 1)];
    memmove(mInsert, insert_mark, (lines_affected * sizeof(PhxMark)));
    lines_affected--;
      // guard against possible overwrite '(++drop_mark)->offset'
    PhxMark *drop_preserve = drop_mark;

      // move full lines between release and drop
    int delta = tv->release.offset - tv->insert.offset;
    while (insert_mark > drop_mark) {
      release_mark->offset = insert_mark->offset + delta;
      --release_mark, --insert_mark;
    }

    if (lines_affected != 0) {
      drop_mark = drop_preserve;
      (drop_mark + 1)->offset = tv->drop.offset
                               + (mInsert + 1)->offset - tv->insert.offset;
      while ((--lines_affected) != 0) {
        drop_mark++, mInsert++;
        (drop_mark + 1)->offset = drop_mark->offset
                                 + (mInsert + 1)->offset - mInsert->offset;
      }
    }

  } else {

    insert_mark  = bsearch(&tv->insert.offset, tv->newline_list,
                                  tv->list_nil,
                                  sizeof(PhxMark),
                                  _text_marks_offsets_compare);
    drop_mark    = bsearch(&tv->drop.offset, insert_mark,
                                  tv->list_nil - insert_mark->line,
                                  sizeof(PhxMark),
                                  _text_marks_offsets_compare);
    if (insert_mark == drop_mark)  return;

    release_mark = bsearch(&tv->release.offset, insert_mark,
                                  drop_mark->line + 1 - insert_mark->line,
                                  sizeof(PhxMark),
                                  _text_marks_offsets_compare);

    if (insert_mark == release_mark) {
      int delta = tv->release.offset - tv->insert.offset;
      while (insert_mark < drop_mark)
        (++insert_mark)->offset -= delta;
      return;
    }
      // add 1 for cushion for now
    int lines_affected = release_mark->line - insert_mark->line + 1;
    if ((tv->list_nil + lines_affected + 1) >= tv->list_size) {
      puts("realloc: text_marks_DnD_move_update"); return;
    }
      // copies, at min, the next line after insert_mark i=0+1 to i=0+1+la
    PhxMark *mInsert = &tv->newline_list[(tv->list_nil + 1)];
    memmove(mInsert, insert_mark, (lines_affected * sizeof(PhxMark)));
    lines_affected--;
    PhxMark *release_preserve = release_mark;

    int delta = tv->release.offset - tv->insert.offset;
    while (release_mark < drop_mark)
      (++insert_mark)->offset = (++release_mark)->offset - delta;

    if (lines_affected != 0) {
        // remaining length of drop line to drop insert
        // remaining length of initial first line grab
      if (release_preserve == drop_mark)
        delta = tv->drop.offset - tv->release.offset
               + (mInsert + 1)->offset - mInsert->offset;
      else
        delta = tv->drop.offset - drop_mark->offset
               + (mInsert + 1)->offset - tv->insert.offset;
      (insert_mark + 1)->offset = insert_mark->offset + delta;
      while ((--lines_affected) != 0) {
        insert_mark++, mInsert++;
        (insert_mark + 1)->offset = insert_mark->offset
                                   + (mInsert + 1)->offset - mInsert->offset;
      }
    }
  }
}

static void
text_marks_update(PhxObjectTextview *tv) {

  if ((tv->type == PHX_LABEL) || (tv->type == PHX_TEXTBUFFER)) return;
  if (tv->list_dirtyo == INT_MAX)  return;

  PhxMark *line_mark = bsearch(&tv->list_dirtyo, tv->newline_list,
                                tv->list_nil, sizeof(PhxMark),
                                _text_marks_offsets_compare);
  char *sPtr = tv->string;
  char *nPtr, *rdPtr = &tv->string[line_mark->offset];

  int ndx = line_mark->line + 1;
  PhxMark *mPtr = line_mark + 1;

  if (tv->gap_start != (tv->str_nil + 1)) {
    char *gsPtr = &tv->string[tv->gap_start];
    char *gePtr = &tv->string[tv->gap_end];
    if (rdPtr < gsPtr)
      do {
        nPtr = strchr(rdPtr, '\n');
        if ((rdPtr <= gsPtr) && (nPtr >= gsPtr))  break;
        if (nPtr == NULL)  goto lastentry;
        mPtr->offset = (rdPtr = (++nPtr)) - sPtr;
        mPtr->line = ndx;
        if ((++ndx) == tv->list_size) {
          size_t newSz = tv->list_size + MARK_ALLOC;
          PhxMark *newPtr
                      = realloc(tv->newline_list, (newSz * sizeof(PhxMark)));
          if (newPtr == NULL) {
            puts("realloc failure: text_marks_update");
            return;
          }
          tv->newline_list = newPtr;
          tv->list_size = newSz;
          mPtr = &tv->newline_list[(ndx - 1)];
        }
        mPtr++;
      } while (1);
  
    sPtr += (size_t)(gePtr - gsPtr);
    rdPtr = gePtr;
  }
  do {
    nPtr = strchr(rdPtr, '\n');
    if (nPtr == NULL)  break;
    mPtr->offset = (rdPtr = (++nPtr)) - sPtr;
    mPtr->line = ndx;
    if ((++ndx) == tv->list_size) {
      size_t newSz = tv->list_size + MARK_ALLOC;
      PhxMark *newPtr
                  = realloc(tv->newline_list, (newSz * sizeof(PhxMark)));
      if (newPtr == NULL) {
        puts("realloc failure: text_marks_update");
        return;
      }
      tv->newline_list = newPtr;
      tv->list_size = newSz;
      mPtr = &tv->newline_list[(ndx - 1)];
    }
    mPtr++;
  } while (1);

lastentry:
  mPtr->offset = INT_MAX;
  mPtr->line = (tv->list_nil = ndx);

  tv->list_ldelta = 0;
  tv->list_dirtyo = INT_MAX;
}

#endif


#pragma mark *** TextBuffer ***

// given a location
// scroll if needed to view that location
static void
location_auto_scroll(PhxObjectTextview *tv, location *point) {

  int delta;
    // x auto-scroll
  delta = 0;
  int x = point->x;
  int font_em = tv->font_em;
  if (x < (tv->bin.x + font_em)) {
    delta = (tv->bin.x + font_em) - x;
    if (delta > tv->bin.x)  delta = tv->bin.x;
  } else if (x >= (tv->bin.w - font_em)) {
    delta = (tv->bin.w - font_em) - x;
  }
  tv->bin.x -= delta;
  tv->bin.w -= delta;
    // y auto-scroll
  int y = (point->y / font_em) * font_em;
  delta = 0;
  if (y <= tv->bin.y) {
    delta = tv->bin.y - y;
  } else if (y > (tv->bin.h - font_em)) {
    delta = (tv->bin.h - font_em) - y;
  }
  tv->bin.y -= delta;
  tv->bin.h -= delta;
}

// given a offset to text location
// return for temp->offset positions temp->x, temp->y
static void
location_for_offset(PhxObjectTextview *tv, location *temp) {

#if USE_MARKS
  text_marks_update(tv);

  char *gsPtr = &tv->string[tv->gap_start];
  char *gePtr = &tv->string[tv->gap_end];

  PhxMark *line_mark = bsearch(&temp->offset, tv->newline_list,
                                tv->list_nil + 1, sizeof(PhxMark),
                                _text_marks_offsets_compare);
  int x = 0;
  if (line_mark->offset == INT_MAX) {
    line_mark--;
    x = INT_MAX;
  }
  temp->y = line_mark->line * tv->font_em;
  char *rdPtr = &tv->string[line_mark->offset];
  char *textPtr = &tv->string[temp->offset];
  if (textPtr >= gsPtr)  textPtr += gePtr - gsPtr;
  if (rdPtr >= gsPtr)  {
    rdPtr += gePtr - gsPtr;
both_updated:
    while (rdPtr < textPtr)
      x += tv->glyph_widths[(unsigned)(*rdPtr)], rdPtr++;
  } else {
    while (rdPtr < textPtr) {
      x += tv->glyph_widths[(unsigned)(*rdPtr)];
      if ((++rdPtr) == gsPtr) {  rdPtr = gePtr; goto both_updated;  }
    }
  }
  temp->x = x;
#else
  char *rdPtr = tv->string;
  char *gsPtr = &tv->string[tv->gap_start];
  char *gePtr = &tv->string[tv->gap_end];
  char *snPtr = &tv->string[tv->str_nil];
  char *textPtr = &tv->string[temp->offset];
  if (rdPtr >= gsPtr)    rdPtr += gePtr - gsPtr;
  if (textPtr >= gsPtr)  textPtr += gePtr - gsPtr;

  if (rdPtr < textPtr) {
    char *nPtr;
    int font_em = tv->font_em;
    int y = 0;
    do {
      nPtr = strchr(rdPtr, '\n');
      if ((nPtr >= gsPtr) && (rdPtr < gsPtr))
        nPtr = strchr(gePtr, '\n');
      if (nPtr == NULL)    {  nPtr = snPtr;   break;  }
      if (nPtr >= textPtr) {  nPtr = textPtr; break;  }
      if ((rdPtr = nPtr + 1) == gsPtr)  rdPtr = gePtr;
      y += font_em;
    } while (1);

    int x = 0;
      // on editing line
    if (rdPtr < textPtr) {
      if ((rdPtr < gsPtr) && (nPtr > gsPtr)) {
        while (rdPtr < gsPtr)
          x += tv->glyph_widths[(unsigned)(*rdPtr)], rdPtr++;
        rdPtr = gePtr;
      }
      while (rdPtr < textPtr)
        x += tv->glyph_widths[(unsigned)(*rdPtr)], rdPtr++;
    }
    temp->x = x;
    temp->y = y;
    return;
  }
  temp->x = 0;
  temp->y = 0;
#endif
}

// given positions temp->x, temp->y
// return a temp->offset for that position
static void
location_for_point(PhxObjectTextview *tv, location *loc) {

#if USE_MARKS
  text_marks_update(tv);
#endif

  char *nPtr, *rdPtr = tv->string;
  char *gsPtr = &tv->string[tv->gap_start];
  char *gePtr = &tv->string[tv->gap_end];

  int x = loc->x;

#if USE_MARKS
  int y = loc->y / tv->font_em;
  loc->y = 0;
    // used to place with character offset, instead of mouse click
  if (y > 0) {
    PhxMark *line_mark = bsearch(&y, tv->newline_list,
                                 tv->list_nil + 1, sizeof(PhxMark),
                                 _text_marks_lines_compare);
    if (line_mark->offset == INT_MAX) {
      line_mark--;
      x = INT_MAX;
    }
    rdPtr = &tv->string[line_mark->offset];
    loc->y = line_mark->line * tv->font_em;
  }
  if (rdPtr >= gsPtr) rdPtr += gePtr - gsPtr;
  nPtr = strchr(rdPtr, '\n');
  if ((nPtr >= gsPtr) && (rdPtr <= gsPtr))
    nPtr = strchr(gePtr, '\n');
#else
  if (loc->y <= 0) {
    loc->y = 0;
    nPtr = strchr(rdPtr, '\n');
  } else {
    int font_em = tv->font_em;
    int line_no = loc->y / font_em;
    int found_line = 0;
    do {
      nPtr = strchr(rdPtr, '\n');
      if ((nPtr >= gsPtr) && (rdPtr <= gsPtr))
        nPtr = strchr(gePtr, '\n');
      if ((nPtr == NULL) || (found_line == line_no))  break;
      found_line++;
      if ((rdPtr = nPtr + 1) == gsPtr)
        rdPtr = gePtr;
    } while (1);
    if (found_line < line_no)  x = INT_MAX;
    loc->y = (found_line * font_em);
  }
#endif
  if (nPtr == NULL)
    nPtr = &tv->string[tv->str_nil];
  int sum = 0;
  while (rdPtr < nPtr) {
    int gw = tv->glyph_widths[(unsigned)(*rdPtr)];
    if ((sum + gw) > x) {
      if ((x - ((gw + 1) >> 1)) >= sum)  sum += gw, rdPtr++;
      break;
    }
    if ((++rdPtr) == gsPtr)  rdPtr = gePtr;
    if ((sum += gw) == x)  break;
  }
  if (rdPtr > gsPtr)  rdPtr -= gePtr - gsPtr;
  loc->offset = rdPtr - tv->string;
    // used to place with character offset, instead of mouse click
  loc->x = sum;
}

// while editing, gap_buffer moved, set insert info for
// new insert position. This allows delay of having to update
// PhxMark(s) for line_marks
static void
location_update_for_edit(PhxObjectTextview *tv, int sz) {

  if ((tv->type == PHX_LABEL) || (tv->type == PHX_TEXTBUFFER)) return;

#if USE_MARKS
  char *rdPtr = &tv->string[tv->insert.offset];
  if (sz == 0)  goto update_finish;
  if (sz == 1) {
    tv->insert.offset++;
    if (*rdPtr != '\n')
      tv->insert.x += tv->glyph_widths[(unsigned)(*rdPtr)];
    else {
      tv->insert.x = 0;
      tv->insert.y += tv->font_em;
      tv->list_ldelta |= 1;
    }
    goto update_finish;
  }
    // delete 1 character, gap move 'down' 1
  if (sz == -1) {
    tv->insert.offset--;
    if (*(--rdPtr) != '\n')
      tv->insert.x -= tv->glyph_widths[(unsigned)(*rdPtr)];
    else {
      tv->insert.y -= tv->font_em;
      tv->list_ldelta |= 1;
      if (tv->insert.y < 0) {
        tv->insert.x = (tv->insert.y = 0); tv->insert.offset = 0;
        goto update_finish;
      }
      int x = 0;
      while ((--rdPtr) >= tv->string) {
        if (*rdPtr == '\n')  break;
        x += tv->glyph_widths[(unsigned)(*rdPtr)];
      }
      tv->insert.x = x;
    }
    goto update_finish;
  }
  if (sz < 0) {
    printf("    unhandled location_update sz: %d\n", sz);
    goto update_finish;
  }
  //if (sz > 1)
    // sz characters were pasted in
  char *endPtr = &tv->string[(tv->insert.offset + sz)];
  char *nPtr = memchr(rdPtr, '\n', sz);
  if (nPtr != NULL) {
    tv->insert.x = 0;
    tv->insert.y += tv->font_em;
    tv->list_ldelta |= 1;
    do {
      rdPtr = nPtr + 1;
      if (rdPtr >= endPtr)  break;
      nPtr = memchr(rdPtr, '\n', (endPtr - rdPtr));
      if (nPtr == NULL)  break;
      tv->insert.y += tv->font_em;
    } while (1);
  }
  while (rdPtr < endPtr)
    tv->insert.x += tv->glyph_widths[(unsigned)(*rdPtr)], rdPtr++;
  tv->insert.offset += sz;

update_finish:
#else
  location_for_offset(tv, &tv->insert);
#endif
  location_auto_scroll(tv, &tv->insert);
  tv->interim.x = (tv->release.x = tv->insert.x);
  tv->interim.y = (tv->release.y = tv->insert.y);
  tv->interim.offset = (tv->release.offset = tv->insert.offset);
}

static int    findport_search_update_from(LCIFindPort *, int);
static int    findport_display_results(LCIFindPort *, struct _fsearch *);
static struct _fsearch *    findport_results_for(LCIFindPort *, char *);

static void
text_buffer_reset(PhxObjectTextview *otxt) {

  if ((otxt->str_nil + 1) != otxt->gap_start) {
    memmove(&otxt->string[otxt->gap_start], &otxt->string[otxt->gap_end],
                                  (size_t)(otxt->str_nil + 1 - otxt->gap_end));
    int gapSz = otxt->gap_end - otxt->gap_start;
    otxt->str_nil -= gapSz;
    otxt->gap_start = otxt->str_nil + 1;
    otxt->gap_end = otxt->gap_start + gapSz;
    otxt->gap_delta = 0;
  }

  if (otxt->type != PHX_TEXTBUFFER) {
  #if USE_MARKS
    if (otxt->list_dirtyo != INT_MAX)  text_marks_update(otxt);
  #endif
    if (otxt->dirtyo != INT_MAX) {
      if (visible_get((PhxObject*)findPort) == TRUE) {
        struct _fsearch *fdata = findport_results_for(findPort, NULL);
          // note: resets dirtyo
        findport_search_update_from(findPort, otxt->dirtyo);
        findport_display_results(findPort, fdata);
      }
    }
  }
  otxt->dirtyo = INT_MAX;
}

static void
text_buffer_get(char *dst, PhxObjectTextview *otxt, int start, int end) {

  size_t sz = end - start;
    // translate to storage system
  if (start >= otxt->gap_start) {
    start += otxt->gap_end - otxt->gap_start;
  } else if (end > otxt->gap_start) {
      // split copy, accesses gap points
    end += otxt->gap_end - otxt->gap_start;
    sz = otxt->gap_start - start;
    memmove(dst, &otxt->string[start], sz);
    dst += sz;
    start = otxt->gap_end;
    sz = end - start;
  }
  memmove(dst, &otxt->string[start], sz);
  dst[sz] = 0;
}

static void
text_buffer_for_display(char *dst, PhxObjectTextview *otxt,
                                                    location *tempS,
                                                    location *tempE) {
#if USE_MARKS
  if (otxt->list_ldelta != 0)  text_marks_update(otxt);

  PhxMark *line_mark = otxt->newline_list;
  int y = otxt->bin.y / otxt->font_em;
    // used to place with character offset, instead of mouse click
  if (y != 0) {
    line_mark = bsearch(&y, otxt->newline_list,
                            otxt->list_nil, sizeof(PhxMark),
                            _text_marks_lines_compare);
    if (line_mark == NULL) {
      printf("requested line: %d of %d available\n", y, otxt->list_nil);
      puts("ERROR: location_for_point() line_mark == NULL");
      return;
    }
    if (line_mark->offset == INT_MAX)  line_mark--;
  }
  tempS->x      = 0;
  tempS->y      = line_mark->line * otxt->font_em;
  tempS->offset = line_mark->offset;

  y = (otxt->bin.h + otxt->font_em) / otxt->font_em;
  line_mark = bsearch(&y, line_mark,
                          otxt->list_nil - line_mark->line,
                          sizeof(PhxMark),
                          _text_marks_lines_compare);
  tempE->x      = 0;  // unused
  tempE->y      = line_mark->line * otxt->font_em;
  if ((line_mark + 1)->offset == INT_MAX)
    tempE->offset = otxt->str_nil;
  else {
    int offset = line_mark->offset;
    if (otxt->list_dirtyo != INT_MAX)
      offset += otxt->gap_start - otxt->dirtyo - otxt->gap_delta;
    tempE->offset = offset;
  }
#else
  tempS->x = 0;
  tempS->y = otxt->bin.y;
  tempE->x = otxt->bin.w;
  tempE->y = otxt->bin.h + otxt->font_em;
  location_for_point(otxt, tempS);
  location_for_point(otxt, tempE);
#endif
  text_buffer_get(dst, otxt, tempS->offset, tempE->offset);
}

static int
_text_buffer_fit(PhxObjectTextview *otxt, int sz) {

  int gapSz = otxt->gap_end - otxt->gap_start;
  if (sz >= gapSz) {
      // make sure gap at end
    text_buffer_reset(otxt);
    int addSz = (sz + TGAP_ALLOC) & ~(TGAP_ALLOC - 1);
    size_t newSz = otxt->str_nil + (gapSz += addSz);
    char *newPtr = realloc(otxt->string_mete, newSz);
    if (newPtr == NULL)  return -1;
    otxt->string = (otxt->string_mete = newPtr);
    memset(&newPtr[otxt->str_nil], 0, gapSz);
    otxt->gap_end = otxt->gap_start + gapSz;
  }
  return gapSz;
}

/* special case. consider as file open write mode with 'w'.
 * used to replace entire existing buffer with 'offscreen' PHX_TEXTBUFFER.
 * A non-edit replacement, gap remains at tail
 */
static void
text_buffer_replace(PhxObjectTextview *otxt, char *data, int sz) {

  if (sz == 0)  return;  // assume error, delete should have been used

  text_buffer_reset(otxt);
  int selSz = otxt->str_nil;
  int gapSz = _text_buffer_fit(otxt, (sz - selSz));
  if (gapSz < 0)  return;

  memmove(otxt->string, data, sz);
  otxt->str_nil = sz;
  otxt->gap_start = otxt->str_nil + 1;
  otxt->gap_end = otxt->gap_start + (gapSz - (sz - selSz));
  otxt->gap_delta = 0;

  if (otxt->type != PHX_TEXTBUFFER) {
    otxt->dirtyo = 0;
  #if USE_MARKS
    otxt->list_ldelta |= 1;
    otxt->list_dirtyo = 0;
    text_marks_update(otxt);
  #endif
    if (visible_get((PhxObject*)findPort) == TRUE) {
      struct _fsearch *fdata = findport_results_for(findPort, NULL);
        // note: resets dirtyo
      findport_search_update_from(findPort, otxt->dirtyo);
      findport_display_results(findPort, fdata);
    }
    otxt->dirtyo = INT_MAX;
  }
}

/* Insert, in addition to normal insert, can do insert into
 * a selection. Normally that is considered a delete than insert.
 * The delete/insert ability is also needed for 'insert' mode.
 * Instead it currently moves gap_end.
 *        Sadly need to alter such that
 * gap_end doesnt move for mark recording.
 */
static void
text_buffer_insert(PhxObjectTextview *otxt, char *data, int sz) {

    // check if inserting into a selection
  int selSz = otxt->release.offset - otxt->insert.offset;
  if ((sz == 0) && (selSz == 0))  return;

  int gapSz = _text_buffer_fit(otxt, (sz - selSz));
  if (gapSz < 0)  return;

#if USE_MARKS
  if (otxt->type != PHX_TEXTBUFFER) {
      // determine if selection has <newline>, 'force update' of marks if so
    if (selSz != 0) {
      otxt->list_ldelta
         |= (memchr(&otxt->string[otxt->insert.offset], '\n', selSz) != NULL);
    } else if ((otxt->state & (0x00000001 << (SHIFT + 10))) != 0) {
      otxt->list_ldelta |= (otxt->string[otxt->insert.offset] == '\n');
    }
  }
#endif

    // case: buffer reset not used
  if (otxt->gap_end < otxt->str_nil) {
    int delta = otxt->gap_start - otxt->release.offset;
    if (delta >= 0) {
      memmove(&otxt->string[(otxt->gap_end - delta)],
              &otxt->string[otxt->release.offset], delta);
    } else {
      memmove(&otxt->string[otxt->gap_start],
              &otxt->string[otxt->gap_end],
                     otxt->insert.offset - otxt->gap_end);
    }
    goto moved;
  }
    // if editing at end, don't move gap but move/add nil byte and gap_start
  if ( (otxt->release.offset != otxt->str_nil)
      && (otxt->insert.offset != otxt->gap_start) ) {
    memmove(&otxt->string[(otxt->release.offset + gapSz)],
            &otxt->string[otxt->release.offset],
              (otxt->str_nil + 1 - otxt->release.offset));
    otxt->str_nil += gapSz;
moved:
    otxt->gap_delta = selSz;
    otxt->gap_end = otxt->release.offset + gapSz;
  }
  if (sz == 1)
    otxt->string[otxt->insert.offset] = *data;
  else
    memmove(&otxt->string[otxt->insert.offset], data, sz);

  if (otxt->dirtyo > otxt->insert.offset) {
    otxt->dirtyo = otxt->insert.offset;
#if USE_MARKS
    otxt->list_dirtyo = otxt->insert.offset;
#endif
  }

#if USE_MARKS
  otxt->gap_start = otxt->insert.offset + sz;
#else
  otxt->insert.offset += sz;
  otxt->gap_start = otxt->insert.offset;
#endif
  if (otxt->release.offset == otxt->str_nil) {
    otxt->str_nil += (sz - selSz);
    otxt->string[otxt->str_nil] = 0;
    otxt->gap_start = otxt->str_nil + 1;
  } else if ((otxt->state & (0x00000001 << (SHIFT + 10))) != 0) {
// need testing looks wrong... explain
    printf("insert mode\n");
    if (selSz != 0)  sz = 0;
    otxt->gap_delta += sz;
    otxt->gap_end += sz;
  }
  location_update_for_edit(otxt, sz);
}

/* For selection, use release instead of insert. */
/* As with text_buffer_insert(), an action, other then editing,
   will 'reset' buffer. */
static void
text_buffer_delete(PhxObjectTextview *otxt) {

    // get info on selection
  int sz = otxt->release.offset - otxt->insert.offset;
    // cases ruled out by size == 0
  if (sz == 0) {
      // nothing to <backspace>
    if (otxt->insert.offset == 0)  return;
  }
    // debugging verify of selection reversal 'no-no'
  if (sz < 0) {  puts("error, text_buffer_delete()"); return;  }

    // if editing at end, don't move gap but move/add nil byte and gap_start
  if (otxt->release.offset == otxt->str_nil) {
      // on 'delete' moves nil sz amount, 0 if sz == 0
    if ((otxt->state & (0x00000001 << (SHIFT + 11))) != 0) {
        // remove flag
      otxt->state ^= (0x00000001 << (SHIFT + 11));
        // nothing to 'delete' at end of buffer, leave locations as is
      if (sz == 0)  return;
    }
#if USE_MARKS
    location_update_for_edit(otxt, -(!sz));
      // need end adjusted
    if (otxt->dirtyo > otxt->insert.offset)
      otxt->list_dirtyo = (otxt->dirtyo = otxt->insert.offset);
    otxt->str_nil += -(sz + !sz);
    otxt->string[otxt->str_nil] = 0;
    otxt->gap_start = otxt->str_nil + 1;
    return;
#else
    otxt->str_nil += -(sz + !sz);
    otxt->string[otxt->str_nil] = 0;
    otxt->insert.offset -= !sz;
    if (otxt->dirtyo > otxt->insert.offset)
      otxt->dirtyo = otxt->insert.offset;
    otxt->gap_start = otxt->str_nil + 1;
#endif
  } else {
    if (otxt->insert.offset != otxt->gap_start) {
      int gapSz = otxt->gap_end - otxt->gap_start;
        // move release the distance of gapSz, include nil with move
      memmove(&otxt->string[(otxt->release.offset + gapSz)],
              &otxt->string[otxt->release.offset],
              (otxt->gap_start - otxt->release.offset));
      otxt->gap_delta = sz;
      otxt->gap_end = otxt->release.offset + gapSz;
      otxt->str_nil += gapSz;
#if USE_MARKS
      if (otxt->type != PHX_TEXTBUFFER) {
        if (sz != 0) {
          otxt->list_ldelta
           |= (memchr(&otxt->string[otxt->insert.offset], '\n', sz) != NULL);
        }
      }
#endif
    }
    if ((otxt->state & (0x00000001 << (SHIFT + 11))) != 0) {
      otxt->state ^= (0x00000001 << (SHIFT + 11));
      if (!sz)  otxt->gap_end++, otxt->gap_delta++, sz++;
    }
#if USE_MARKS
    otxt->gap_start = otxt->insert.offset - !sz;
    if (otxt->dirtyo > (otxt->insert.offset - !sz))
      otxt->list_dirtyo = (otxt->dirtyo = (otxt->insert.offset - !sz));
#else
    otxt->insert.offset -= !sz;
    if (otxt->dirtyo > otxt->insert.offset)
      otxt->dirtyo = otxt->insert.offset;
    otxt->gap_start = otxt->insert.offset;
#endif
  }
  location_update_for_edit(otxt, -(!sz));
}

static void
text_buffer_select_all(PhxObjectTextview *otxt) {

  text_buffer_reset(otxt);
  otxt->insert.offset = 0;
  otxt->release.offset = otxt->str_nil;
  location_for_offset(otxt, &otxt->insert);
  location_for_offset(otxt, &otxt->release);
  cairo_region_t *crr;
  crr = cairo_region_create_rectangle(
           (cairo_rectangle_int_t *)&otxt->draw_box);
  gdk_window_invalidate_region(otxt->iface->parent_window, crr, FALSE);
  cairo_region_destroy(crr);
}

static void
text_buffer_copy(PhxObjectTextview *otxt) {

  int sz = otxt->release.offset - otxt->insert.offset;
  gtk_clipboard_set_text(clipboard, &otxt->string[otxt->insert.offset], sz);
}

static void
text_buffer_cut(PhxObjectTextview *otxt) {

  int sz = otxt->release.offset - otxt->insert.offset;
  gtk_clipboard_set_text(clipboard, &otxt->string[otxt->insert.offset], sz);
  text_buffer_delete(otxt);
}

static void
text_buffer_paste(PhxObjectTextview *otxt) {

  gchar *clip_string = gtk_clipboard_wait_for_text(clipboard);
  if (clip_string != NULL) {
    text_buffer_insert(otxt, clip_string, (int)strlen(clip_string));
    g_free(clip_string);
  }
}

static void
text_buffer_board_set(PhxObjectTextview *receiver, char *data, size_t sz) {

    // text_buffer_clear()
  receiver->str_nil = 0;
  receiver->insert.offset = 0;
  receiver->gap_start = 1;

  location_for_offset(receiver, &receiver->insert);
  location_auto_scroll(receiver, &receiver->insert);
  receiver->interim.x = (receiver->release.x = receiver->insert.x);
  receiver->interim.y = (receiver->release.y = receiver->insert.y);
  receiver->interim.offset
                      = (receiver->release.offset = receiver->insert.offset);
    // non-gap_move insert, adjust gapSz
  int gapSz = _text_buffer_fit(receiver, sz);
  if (gapSz < 0)  return;
  memmove(receiver->string, data, (size_t)sz);
  receiver->string[sz] = 0;
  receiver->str_nil = sz;
  receiver->gap_start = sz + 1;
  receiver->dirtyo = INT_MAX;

#if USE_MARKS
  text_marks_initialize_for(receiver);
#endif

  cairo_region_t *crr;
  crr = cairo_region_create_rectangle(
           (cairo_rectangle_int_t *)&receiver->draw_box);
  gdk_window_invalidate_region(receiver->iface->parent_window, crr, FALSE);
  cairo_region_destroy(crr);
}

/*
    DnD marks: No edit, no gap set up.
    Move: There is no sz change of marks, just offsets.
   Forward Move:
     Alter from locations insert to drop as delete.
     Then locations drop to list_nil as sz of copied insert.
   Backward Move:
     Alter from locations insert to list_nil, sz of copied, as delete.
     Then locations drop to list_nil, sz of copied, as insert.
    In DnD copy: No edit, no gap set up.
    Copy: There is a sz change of marks. No deletion.
*/
static BState  ui_interface_mouse_get(PhxInterface *);

static void
text_buffer_drag_release(PhxObjectTextview *otxt) {

    // release outside valid drops
  if (otxt->drop.offset == otxt->interim.offset) {
    location_auto_scroll(otxt, &otxt->insert);
    return;
  }

    // 'gap' should be at tail, use as copy buffer
  int sz = otxt->release.offset - otxt->insert.offset;
    // add 1 to sz for nil at end of copy to gap
  if (_text_buffer_fit(otxt, (sz + 1)) < 0)  return;

  memmove(&otxt->string[otxt->gap_start],
            &otxt->string[otxt->insert.offset], (size_t)sz);
    // make copied a c-string
  otxt->string[(otxt->gap_start + sz)] = 0;

  if (ui_interface_mouse_get(otxt->iface) == DND_COPY_CURSOR) {
    otxt->gap_start += sz;
    memmove(&otxt->string[(otxt->drop.offset + sz)],
            &otxt->string[otxt->drop.offset],
            (size_t)(otxt->gap_start - otxt->drop.offset));
    memmove(&otxt->string[otxt->drop.offset],
              &otxt->string[otxt->gap_start], (size_t)sz);
    otxt->str_nil += sz;
#if USE_MARKS
    text_marks_DnD_copy_update(otxt);
#else
    otxt->drop.offset += sz;
#endif
  } else {
    if (otxt->drop.offset < otxt->insert.offset) {
      memmove(&otxt->string[(otxt->drop.offset + sz)],
              &otxt->string[otxt->drop.offset],
              (size_t)(otxt->insert.offset - otxt->drop.offset));
      memmove(&otxt->string[otxt->drop.offset],
                &otxt->string[otxt->gap_start], (size_t)sz);
    } else if (otxt->drop.offset > otxt->release.offset) {
      memmove(&otxt->string[otxt->insert.offset],
              &otxt->string[otxt->release.offset],
              (size_t)(otxt->drop.offset - otxt->release.offset));
      memmove(&otxt->string[(otxt->drop.offset - sz)],
                &otxt->string[otxt->gap_start], (size_t)sz);
    } else
      return;
#if USE_MARKS
    text_marks_DnD_move_update(otxt);
    if (otxt->drop.offset > otxt->release.offset)
      otxt->drop.offset -= sz;
#else
    if (otxt->drop.offset < otxt->insert.offset)
      otxt->drop.offset += sz;
#endif
  }

    // different than 'insert'/'delete', no edit/gap assigned
    // also leaves drop hilighted

#if USE_MARKS
    // without hilight, don't subtract sz, or forward drops
  otxt->insert.offset = otxt->drop.offset;
  location_for_offset(otxt, &otxt->insert);
  otxt->release.offset = otxt->insert.offset + sz;
  location_for_offset(otxt, &otxt->release);
  otxt->interim.x = otxt->release.x;
  otxt->interim.y = otxt->release.y;
  otxt->interim.offset = otxt->release.offset;
#else
    // without hilight, don't subtract sz, forward drops add sz
  otxt->insert.offset = otxt->drop.offset;
  location_for_offset(otxt, &otxt->insert);
  location_auto_scroll(otxt, &otxt->insert);
  otxt->interim.x = (otxt->release.x = otxt->insert.x);
  otxt->interim.y = (otxt->release.y = otxt->insert.y);
  otxt->interim.offset = (otxt->release.offset = otxt->insert.offset);
#endif
}

static void
text_buffer_apply_search_tag(LCIFindPort *fport, struct _fsearch *searches) {

  PhxObjectTextview *textview = (PhxObjectTextview*)fport->param_data;
  int majdx = searches->qdx >> 5;
  int mnrdx = searches->qdx & ~(-1 << 5);
  textview->insert.offset = (searches->q_results + majdx)->qoffsets[mnrdx];
  location_for_offset(textview, &textview->insert);
  location_auto_scroll(textview, &textview->insert);
  textview->interim.x = textview->insert.x;
  textview->interim.y = textview->insert.y;
  textview->interim.offset = textview->insert.offset;
  PhxObjectTextview *key_object
         = (PhxObjectTextview*)fport->object_list[textview_find_box];
  textview->release.offset = textview->insert.offset + key_object->str_nil;
  location_for_offset(textview, &textview->release);
  cairo_region_t *crr;
  crr = cairo_region_create_rectangle(
           (cairo_rectangle_int_t *)&textview->mete_box);
  gdk_window_invalidate_region(textview->iface->parent_window, crr, FALSE);
  cairo_region_destroy(crr);
}

#pragma mark *** Interface ***

static PhxObject *ui_object_create(PhxObjectType, PhxDrawHandler,
                                                int, int, int, int);
static PhxInterface *
ui_interface_create(GtkDrawingArea *da, int x, int y, int w, int h) {

  PhxInterface *port = malloc(sizeof(PhxInterface));

  port->type = PHX_IFACE;
  port->state = 0;
  port->mete_box.x = x;
  port->mete_box.y = y;
  port->mete_box.w = w;
  port->mete_box.h = h;
  port->parent_window = gtk_widget_get_window(GTK_WIDGET(da));

  GdkRectangle workarea = {0};
  gdk_monitor_get_workarea(
            gdk_display_get_primary_monitor(gdk_display_get_default()),
            &workarea);

  port->object_list = malloc(OBJECTS_MAX * sizeof(void*));
  port->map_size = workarea.width * 2048;
  port->event_map = malloc(port->map_size);
  memset(port->object_list, 0, (OBJECTS_MAX * sizeof(void*)));
  memset(port->event_map, 0, port->map_size);
    // not ready to use 'expose' event to replace gtk "draw", pass NULL
  port->object_list[0] = ui_object_create(PHX_DRAWING, NULL, x, y, w, h);

  port->has_focus = NULL;
  port->reserved[3] = (port->reserved[4] = NULL);
  port->reserved[1] = (port->reserved[2] = NULL);
  port->reserved[0] = NULL;

  return port;
}

static void
ui_interface_refresh(PhxInterface *iface) {

  memset(iface->event_map, 0, iface->map_size);

  int ldx = 0;
  PhxObject *obj;
  while ((obj = iface->object_list[(++ldx)]) != NULL) {
    int sxdx = obj->draw_box.x,
        sydx = obj->draw_box.y;
    int exdx = sxdx + obj->draw_box.w,
        eydx = sydx + obj->draw_box.h;
    for (int x = sxdx; x <= exdx; x++) {
      for (int y = sydx; y <= eydx; y++)
        iface->event_map[x][y] = (char)ldx;
    }
  }
}

static void
ui_interface_add(PhxInterface *interface, PhxObject *obj) {

  int ldx = 1;
  while ( (interface->object_list[ldx] != NULL)
            && (interface->object_list[ldx] != obj) )
    ldx++;
  obj->iface = interface;
  interface->object_list[ldx] = obj;
  int sxdx = obj->draw_box.x, sydx = obj->draw_box.y;
  int exdx = sxdx + obj->draw_box.w,
      eydx = sydx + obj->draw_box.h;
  for (int x = sxdx; x <= exdx; x++) {
    for (int y = sydx; y <= eydx; y++)
      interface->event_map[x][y] = (char)ldx;
  }
}

static void
ui_interface_set_cursor_named(GdkWindow *window, char *named) {

  GdkCursor *cursor = gdk_window_get_cursor(window);
  if (cursor != NULL)  g_object_unref((void*)cursor);
  cursor = NULL;
  if (named != NULL) {
    GdkDisplay *display = gdk_window_get_display(window);
    cursor = gdk_cursor_new_from_name(display, named);
  }
  gdk_window_set_cursor(window, cursor);
}

static void
ui_interface_mouse_set(PhxInterface *iface, BState value) {
  iface->state = (iface->state & ~15) | value;
}

static BState
ui_interface_mouse_get(PhxInterface *iface) {
  return (iface->state & 15);
}

#pragma mark *** Drawing ***

static void
draw_textview(PhxObject *b, cairo_t *cr) {

  PhxObjectTextview *tv = (PhxObjectTextview*)b;
  char *draw_buffer = tv->draw_buffer;

  cairo_set_source_rgba(cr, 1, 1, 1, 1);
  cairo_rectangle(cr, tv->mete_box.x, tv->mete_box.y,
                      tv->mete_box.w, tv->mete_box.h);
  cairo_fill(cr);

  if ((tv->string == NULL) || (*tv->string == 0))  return;

  double x = tv->draw_box.x,
         y = tv->draw_box.y;
  double font_em = tv->font_em;

  cairo_save(cr);

  cairo_rectangle(cr, x, y, tv->draw_box.w, tv->draw_box.h);
  cairo_clip(cr);

  cairo_select_font_face(cr, tv->font_name, CAIRO_FONT_SLANT_NORMAL,
                                               CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr, tv->font_size);

  location tempS, tempE;
  text_buffer_for_display(draw_buffer, tv, &tempS, &tempE);

  x -= tv->bin.x;
  y -= tv->bin.y;
  double glyph_origin = tv->font_origin;
  double endline = tempE.y + font_em + y;
  y += tempS.y;

  _Bool focused = (tv == (PhxObjectTextview*)tv->iface->has_focus);
  double colour = 0.0;
  if ( (!focused) || (!sensitive_get((PhxObject*)tv)) )
    colour = 0.5;
  cairo_set_source_rgba(cr, colour, colour, colour, 1);
  char *tPtr = draw_buffer;
  do {
    char *nPtr = strchr(tPtr, '\n');
    if (nPtr != NULL)  *nPtr = 0;
    cairo_move_to(cr, x, (y + glyph_origin));
      // repace tabs with movement
    do {
      char *tab = strchr(tPtr, '\t');
      if (tab == NULL)  break;
      *tab = 0;
      cairo_show_text(cr, tPtr);
      cairo_rel_move_to(cr, tv->glyph_widths['\t'], 0);
        // re-insert back for case: insert block caret draw size
      *tab = '\t';
      tPtr = tab + 1;
    } while (1);
    cairo_show_text(cr, tPtr);
    if (nPtr == NULL)  break;
    if ((y += font_em) >= endline)  break;
    tPtr = nPtr + 1;
  } while (1);

  double x0, x1, y0, y1;
  x0 = tv->insert.x + tv->draw_box.x - tv->bin.x;
  x1 = tv->release.x + tv->draw_box.x - tv->bin.x;
  y0 = tv->insert.y + tv->draw_box.y - tv->bin.y;
  y1 = tv->release.y + tv->draw_box.y - tv->bin.y;

  if (tv->insert.offset == tv->release.offset) {
    if ( focused && sensitive_get((PhxObject*)tv) ) {
      if ((tv->state & (0x00000001 << (SHIFT + 10))) != 0) {
        int utf = draw_buffer[(tv->insert.offset - tempS.offset)];
        cairo_set_source_rgb(cr, 0, 0, 0);
        cairo_rectangle(cr, x0, y0, tv->glyph_widths[utf], font_em);
        cairo_fill(cr);
        cairo_set_source_rgb(cr, 1, 1, 1);
        cairo_move_to(cr, x0, y0 + glyph_origin);
        if (utf != '\t')
          cairo_show_text(cr, (const char*)&utf);
      } else {
        cairo_set_source_rgb(cr, 0, 0, 0);
        cairo_rectangle(cr, x0, y0, 0.5, font_em);
        cairo_fill(cr);
      }
    }
  } else {
      // if dnd draw caret drop location (only if outside selection)
    if ( ((ui_interface_mouse_get(tv->iface) & MOUSE_DND) == MOUSE_DND)
        && (tv->drop.offset != tv->interim.offset) ) {
      double dx, dy;
      dx = tv->drop.x + tv->draw_box.x - tv->bin.x;
      dy = tv->drop.y + tv->draw_box.y - tv->bin.y;
      cairo_set_source_rgb(cr, 0, 0, 0);
      cairo_rectangle(cr, dx, dy, 0.5, font_em);
      cairo_fill(cr);
    }
      // ? colour change if not focus
    colour = 1.0;
    if ( !focused || !(sensitive_get((PhxObject*)tv)) )
      colour = 0.5;
    cairo_set_source_rgba(cr, 0, 0, colour, .2);
    if (tv->insert.y == tv->release.y) {
      cairo_rectangle(cr, x0, y0, (x1 - x0), font_em);
      cairo_fill(cr);
    } else {
      if (y0 > y1) {
          // needed for mouse movement, swap
        endline = x1, x1 = x0, x0 = endline;
        endline = y1, y1 = y0, y0 = endline;
      }
      if (x0 < tv->bin.x)  x0 = tv->draw_box.x;
      cairo_rectangle(cr, x0, y0, tv->draw_box.w, font_em);
      if ((endline = y0 + font_em) < y1)
        do {
          cairo_rectangle(cr, tv->draw_box.x, endline,
                              tv->draw_box.w, font_em);
        } while ((endline += font_em) < y1);
      if (x1 > tv->draw_box.x) {
        cairo_rectangle(cr, tv->draw_box.x, y1,
                            (x1 - tv->draw_box.x), font_em);
      }
      cairo_fill(cr);
    }
  }
  cairo_restore(cr);
}

static void
draw_label(PhxObject *b, cairo_t *cr) {

  PhxObjectTextview *label = (PhxObjectTextview*)b;

  if ((label->string == NULL) || (*label->string == 0))  return;

  cairo_select_font_face(cr, label->font_name, CAIRO_FONT_SLANT_NORMAL,
                                               CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr, label->font_size);
  cairo_text_extents_t search_extents;
  cairo_text_extents(cr, label->string, &search_extents);

  int bx = label->bin.x,
      by = label->bin.y;
  int whtsp = label->draw_box.w - search_extents.x_advance;
  if (whtsp > 0) {
    int position = label->state & HJST_MSK;
    if      (position == HJST_LFT) {  whtsp = 0;  }
    else if (position == HJST_CTR) {  whtsp /= 2; }
    bx -= whtsp;
  }

  double x = label->draw_box.x,
         y = label->draw_box.y;

  cairo_save(cr);
  cairo_rectangle(cr, x, y, label->draw_box.w, label->draw_box.h);
  cairo_clip(cr);

  cairo_set_source_rgba(cr, 0, 0, 0, 1);
  cairo_move_to(cr, x - bx, y + label->font_origin - by);
  cairo_show_text(cr, label->string);

  cairo_clip_preserve(cr);
  cairo_restore(cr);
}

static void
draw_button(PhxObject *b, cairo_t *cr) {

  PhxObjectButton *button = (PhxObjectButton *)b;
  const double degrees = M_PI / 180.0;
  double radius = button->draw_box.h / 6.5;

  double line_width = 0.5;
  double x = button->draw_box.x + line_width;
  double y = button->draw_box.y + line_width;
  double w = button->draw_box.w - (line_width * 2);
  double h = button->draw_box.h - (line_width * 2);

  cairo_new_sub_path(cr);
  cairo_arc(cr, x + w - radius, y + radius,
                radius, -90 * degrees,   0 * degrees);
  cairo_arc(cr, x + w - radius, y + h - radius,
                radius,   0 * degrees,  90 * degrees);
  cairo_arc(cr, x + radius, y + h - radius,
                radius,  90 * degrees, 180 * degrees);
  cairo_arc(cr, x + radius, y + radius,
                radius, 180 * degrees, 270 * degrees);
  cairo_close_path(cr);

  cairo_set_source_rgba(cr, 0.94, 0.94, 0.94, 1);
  cairo_fill_preserve(cr);

  if ((button->state & 1) == 1) {
    cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
    cairo_fill_preserve(cr);
  }

  cairo_set_source_rgba(cr, 0, 0, 0, 1);
  cairo_set_line_width(cr, line_width);
  cairo_stroke(cr);

    // shade bottom
  cairo_set_source_rgba(cr, 0.7, 0.7, 0.7, 0.25);
  cairo_rectangle(cr, x, y + h - 2 + line_width, w, 2);
  cairo_fill(cr);
}

static void
draw_button_combo_arrows(PhxObject *b, cairo_t *cr) {

  PhxObjectDrawing *odrw = (PhxObjectDrawing*)b;

  double line_width = 0.5;
  double x = odrw->draw_box.x;
  double y = odrw->draw_box.y - line_width;
  double w = odrw->draw_box.w;
  double h = odrw->draw_box.h + line_width;

  cairo_new_path(cr);  /* current path is not consumed by another */
  cairo_set_source_rgba(cr, 0.25, 0.25, 0.25, 1);
  cairo_move_to(cr, (x + (w/2)),  y);
  cairo_line_to(cr, (x + w),     (y + (h/2) - .5));
  cairo_line_to(cr,  x,          (y + (h/2) - .5));
  cairo_line_to(cr, (x + (w/2)),  y);
  cairo_move_to(cr, (x + (w/2)), (y + h));
  cairo_line_to(cr, (x + w),     (y + (h/2) + .5));
  cairo_line_to(cr,  x,          (y + (h/2) + .5));
  cairo_line_to(cr, (x + (w/2)), (y + h));
  cairo_fill(cr);
}

static void
draw_navigate(PhxObject *b, cairo_t *cr) {

  PhxObjectDrawing *odrw = (PhxObjectDrawing*)b;
  const double degrees = M_PI / 180.0;
  double radius = odrw->draw_box.h / 6.5;

  double line_width = 0.5;
  double x = odrw->draw_box.x + line_width;
  double y = odrw->draw_box.y + line_width;
  double w = odrw->draw_box.w - (line_width * 2);
  double h = odrw->draw_box.h - (line_width * 2);
  double sp = h / 5;

  if (odrw->type == PHX_NAVIGATE_RIGHT) {

    cairo_new_sub_path(cr);
    cairo_arc(cr, x + w - radius, y + radius,
                  radius, -90 * degrees,   0 * degrees);
    cairo_line_to(cr, x + w, y + h - radius);
    cairo_arc(cr, x + w - radius, y + h - radius,
                  radius,   0 * degrees,  90 * degrees);
    cairo_line_to(cr, x, y + h);
    cairo_line_to(cr, x, y);
    cairo_line_to(cr, x + w - radius, y);
    cairo_close_path(cr);

    cairo_set_source_rgba(cr, 0.94, 0.94, 0.94, 1);
    cairo_fill_preserve(cr);

    if ((odrw->state & 1) == 1) {
      cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
      cairo_fill_preserve(cr);
    }

    double colour = (sensitive_get((PhxObject*)odrw)) ? 0.0 : 0.5;
    cairo_set_source_rgba(cr, colour, colour, colour, 1);
    cairo_set_line_width(cr, .5);
    cairo_stroke(cr);

    colour += 0.2;
    cairo_set_source_rgba(cr, colour, colour, colour, 1);
    cairo_move_to(cr, (x + w - sp), (y + (h / 2)));
    cairo_line_to(cr,  x + sp,       y + sp);
    cairo_line_to(cr,  x + sp,      (y + h - sp));
    cairo_line_to(cr, (x + w - sp), (y + (h / 2)));
    cairo_fill(cr);

  } else if (odrw->type == PHX_NAVIGATE_LEFT) {

    w = odrw->draw_box.w - line_width;

    cairo_new_sub_path(cr);
    cairo_arc(cr, x + radius, y + radius,
                  radius, 180 * degrees, 270 * degrees);
    cairo_line_to(cr, x + w, y);
    cairo_line_to(cr, x + w, y + h);
    cairo_line_to(cr, x + radius , y + h);
    cairo_arc(cr, x + radius, y + h - radius,
                  radius,  90 * degrees, 180 * degrees);
    cairo_line_to(cr, x, y + radius);
    cairo_close_path(cr);

    cairo_set_source_rgba(cr, 0.94, 0.94, 0.94, 1);
    cairo_fill_preserve(cr);

    if ((odrw->state & 1) == 1) {
      cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
      cairo_fill_preserve(cr);
    }

    double colour = (sensitive_get((PhxObject*)odrw)) ? 0.0 : 0.5;
    cairo_set_source_rgba(cr, colour, colour, colour, 1);
    cairo_set_line_width(cr, .5);
    cairo_stroke(cr);

    colour += 0.2;
    cairo_set_source_rgba(cr, colour, colour, colour, 1);
    cairo_move_to(cr,  x + sp,      (y + (h / 2)));
    cairo_line_to(cr, (x + w - sp),  y + sp);
    cairo_line_to(cr, (x + w - sp), (y + h) - sp);
    cairo_line_to(cr,  x + sp,      (y + (h / 2)));
    cairo_fill(cr);

  }

    // shade bottom
  cairo_set_source_rgba(cr, 0.7, 0.7, 0.7, 0.25);
  cairo_rectangle(cr, x, y + h - 2 + line_width, w, 2);
  cairo_fill(cr);
}

static gboolean
text_draw_event(PhxInterface *tport, cairo_t *cr, void *widget) {

  (void)widget;
  cairo_reset_clip(cr);

     // only 1 object to tport
  PhxObject *obj = tport->object_list[1];
  obj->_draw_cb(obj, cr);
  return TRUE;
}

static gboolean
find_draw_event(PhxInterface *fport, cairo_t *cr, void *widget) {

  (void)widget;
  cairo_reset_clip(cr);

    // interface background
  cairo_set_source_rgba(cr, 0.94, 0.94, 0.94, 1);
  cairo_rectangle(cr, 0, 0, fport->mete_box.w, fport->mete_box.h);
  cairo_clip(cr);
  cairo_fill(cr);

    // draw each object, after 0 (the drawing window)
    // 0 just shades background, see above
  cairo_save(cr);
    int ldx = 0;
    while (fport->object_list[(++ldx)] != NULL) {
      PhxObject *obj = fport->object_list[ldx];
      if (visible_get(obj)) {
        if (obj->_draw_cb != NULL)  obj->_draw_cb(obj, cr);
        if (obj->child != NULL) {
            // walk through any children
          PhxObject *add = obj->child;
          do {
            if (add->_draw_cb != NULL)  add->_draw_cb(add, cr);
            add = add->child;
          } while (add != NULL);
        }
      }
    }
  cairo_restore(cr);
  return TRUE;
}

#pragma mark *** Objects ***

static PhxObject *
ui_object_create(PhxObjectType type, PhxDrawHandler draw,
                                               int x, int y, int w, int h) {

  if ((type >= PHX_OBJECT_LAST) || (type < 0))  return NULL;

  PhxObject *obj;

  size_t sz;
  if ((type == PHX_TEXTVIEW)
          || (type == PHX_ENTRY)
          || (type == PHX_LABEL)
          || (type == PHX_TEXTBUFFER)) {
    sz = sizeof(PhxObjectTextview);
  } else if ((type == PHX_BUTTON)
          || (type == PHX_BUTTON_LABELED)
          || (type == PHX_BUTTON_COMBO)) {
    sz = sizeof(PhxObjectButton);
  } else {
    sz = sizeof(PhxObject);
  }
  obj = malloc(sz);
  memset(obj, 0, sz);

  obj->type = type;
  obj->draw_box.x = (obj->mete_box.x = x);
  obj->draw_box.y = (obj->mete_box.y = y);
  obj->draw_box.w = (obj->mete_box.w = w);
  obj->draw_box.h = (obj->mete_box.h = h);
  obj->_draw_cb = draw;

  if ((type == PHX_TEXTVIEW) || (type == PHX_ENTRY)) {
    PhxObjectTextview *otxt = (PhxObjectTextview*)obj;
      // 8.5" page @ 72 p/i with avg 7.5 p/char = 82 chars
      // 11" => 792p or 66 lines
      // based on 82 chars with 1080p @ 12pixels/line = 7380
    otxt->draw_buffer = malloc(BUFF_ALLOC);
    memset(otxt->draw_buffer, 0, BUFF_ALLOC);
  }
  return obj;
}

static void
ui_box_inset(PhxRectangle *box, int l, int t, int r, int b) {
  box->x += l;
  box->y += t;
  box->w -= l + r;
  box->h -= t + b;
}

static void
ui_textview_font_set(PhxObjectTextview *otxt, cairo_t *cro, int line_height) {

  if ((otxt->type != PHX_TEXTVIEW) && (otxt->type != PHX_ENTRY)
      && (otxt->type != PHX_LABEL))
    return;

  cairo_select_font_face(cro, FONT_NAME, CAIRO_FONT_SLANT_NORMAL,
                                               CAIRO_FONT_WEIGHT_NORMAL);

  cairo_font_extents_t font_extents;
  double pixel_line_height = line_height;
  double font_size = line_height;
  cairo_set_font_size(cro, pixel_line_height);
  cairo_font_extents(cro, &font_extents);
  while (pixel_line_height
          < (int)(font_extents.ascent + font_extents.descent)) {
    font_size -= 0.5;
    cairo_set_font_size(cro, font_size);
    cairo_font_extents(cro, &font_extents);
  }
  otxt->font_name   = FONT_NAME;
  otxt->font_size   = font_size;
  otxt->font_origin = font_extents.ascent;
  otxt->font_em     = (int)(font_extents.ascent + font_extents.descent);

  if (otxt->type != PHX_LABEL) {

    otxt->glyph_widths = malloc(128 * sizeof(char));
    memset(otxt->glyph_widths, 0, 0x20);

    for (int idx = 0x20; idx <= 0x7e; idx++) {
      cairo_text_extents_t search_extents;
      int utf_str = idx;
      cairo_text_extents(cro, (const char*)&utf_str, &search_extents);
      otxt->glyph_widths[idx] = (unsigned)(search_extents.x_advance + 0.5);
    }
    otxt->glyph_widths[0x7f] = 1;
      // want a size for good representation with block_caret
    otxt->glyph_widths[0]    = otxt->glyph_widths[0x20];
    otxt->glyph_widths['\t'] = otxt->glyph_widths[0x20] << 1;
  }

}

static void
ui_textview_buffer_set(PhxObjectTextview *otxt, char *data) {

  if ((otxt->type != PHX_TEXTVIEW) && (otxt->type != PHX_ENTRY)
      && (otxt->type != PHX_LABEL) && (otxt->type != PHX_TEXTBUFFER))
    return;

  if (otxt->string_mete != NULL)  free(otxt->string_mete);

  if (otxt->type != PHX_TEXTBUFFER) {
    otxt->bin.w = otxt->draw_box.w;
    otxt->bin.h = otxt->draw_box.h;
    if (otxt->type == PHX_LABEL) {
      if (data != NULL)
            otxt->string_mete = strdup(data);
      else  otxt->string_mete = strdup("");
      otxt->string = otxt->string_mete;
      return;
    }
  }

  size_t rdSz = (data != NULL) ? strlen(data) : 0;
  size_t bufSz = (rdSz + TGAP_ALLOC) & ~(TGAP_ALLOC - 1);
  if ((otxt->type == PHX_TEXTVIEW) || (otxt->type == PHX_TEXTBUFFER))
    if ((bufSz - rdSz) < (TGAP_ALLOC >> 1))  bufSz += TGAP_ALLOC;
  otxt->string_mete = malloc(bufSz);
  otxt->string = otxt->string_mete;
  memset(&((char*)otxt->string_mete)[rdSz], 'a', (bufSz - rdSz));
  if (rdSz)
    memmove(otxt->string_mete, data, rdSz);
  ((char*)otxt->string_mete)[rdSz] = 0;
  otxt->str_nil = rdSz;
  otxt->gap_start = rdSz + 1;
  otxt->gap_end = bufSz - 1;

  otxt->dirtyo = INT_MAX;
#if USE_MARKS
  if (otxt->type != PHX_TEXTBUFFER)
    text_marks_initialize_for(otxt);
#endif
}

/* returns white space of label in drawing box */
static int
ui_label_set(PhxObjectLabel *olbl, cairo_t *cro, char *str, int position) {

  if (olbl->type != PHX_LABEL)  return -1;

  ui_textview_font_set(olbl, cro, olbl->draw_box.h);
  ui_textview_buffer_set(olbl, str);
  olbl->state &= ~HJST_MSK;
  olbl->state |= position;

  cairo_text_extents_t search_extents;
  cairo_text_extents(cro, olbl->string, &search_extents);
  return olbl->draw_box.w - search_extents.x_advance;
}

static int
ui_button_label_create(PhxObjectButton *obtn, cairo_t *cro,
                                                    char *str, int position) {
  PhxObjectTextview *olbl;
  olbl = (PhxObjectTextview*)ui_object_create(PHX_LABEL, draw_label,
                                  obtn->draw_box.x + 1, obtn->draw_box.y + 1,
                                  obtn->draw_box.w - 2, obtn->draw_box.h - 2);
  obtn->label = olbl;
  return ui_label_set(olbl, cro, str, position);
}

static void
ui_button_menu_create(PhxObjectButton *obtn, cairo_t *cro,
                                           int number_strings, ...) {
  PhxObjectTextview *olbl;
  olbl = (PhxObjectTextview*)ui_object_create(PHX_LABEL, draw_label,
                                  obtn->draw_box.x + 1, obtn->draw_box.y + 1,
                                  obtn->draw_box.w - 2, obtn->draw_box.h - 2);
  obtn->label = olbl;
    // note: leaving string_mete and string as NULL
  ui_textview_font_set(olbl, cro, olbl->draw_box.h);
  ui_textview_buffer_set(olbl, NULL);

  int max_len = 0;
  if (number_strings > 0) {
    int scnt = 0;
    olbl->string_mete = malloc(sizeof(char*) * number_strings);
    va_list arg;
    va_start(arg, number_strings);
      char **sHnd = olbl->string_mete;
      do {
        char *str = va_arg(arg, char*);
        sHnd[scnt] = strdup(str);
      } while ((++scnt) < number_strings);
    va_end(arg);
      // now find longest display of strings
      // reminder: a label can be multi-line
    scnt = number_strings;
    do {
      cairo_text_extents_t search_extents;
      char *sPtr = ((char**)olbl->string_mete)[(--scnt)];
      do {
        char *nPtr = strchr(sPtr, '\n');
        if (nPtr != NULL)  *nPtr = 0;
        cairo_text_extents(cro, sPtr, &search_extents);
        if ((int)search_extents.x_advance > max_len) {
          max_len = (int)search_extents.x_advance;
        }
        if (nPtr == NULL)  break;
        *nPtr = '\n';
        sPtr = nPtr + 1;
      } while (1);
    } while (scnt);
// should really warn if width conflict
    olbl->string = *((char**)olbl->string_mete);
    olbl->draw_box.x += BUTTON_TEXT_MIN;
  }
  olbl->draw_box.w = (2 * BUTTON_TEXT_MIN) + max_len;
  int end_x = olbl->draw_box.x + olbl->draw_box.w;
    // add on combo's signature arrows
  olbl->child = ui_object_create(PHX_DRAWING, draw_button_combo_arrows,
                                  end_x - BUTTON_TEXT_MIN,
                                  olbl->draw_box.y + 1,
                                  (67.824176 / 152.615385) * olbl->draw_box.h,
                                  olbl->draw_box.h - 2);
    // calculate obtn size based on longest line + signature arrows
  if (olbl->child != NULL) {
    end_x += olbl->child->mete_box.w;
  }
  end_x = (obtn->draw_box.x + obtn->draw_box.w) - end_x;
  ui_box_inset(&obtn->draw_box, 0, 0, end_x, 0);
  ui_box_inset(&obtn->mete_box, 0, 0, end_x, 0);
}

#pragma mark *** FindPort ***

static void
draw_popup(PhxObject *b, cairo_t *cr) {

  cairo_set_source_rgba(cr, .9, .9, .9, 1);
  cairo_rectangle(cr, 0, 0, b->mete_box.w, b->mete_box.h);
  cairo_fill(cr);

  PhxObjectLabel *olbl = (PhxObjectLabel *)b->child;
  cairo_set_source_rgba(cr, 0, 0, 1, .2);
  int y_delta = b->draw_box.y + ((b->state == 0) ? 0 : olbl->font_em);
  cairo_rectangle(cr, b->draw_box.x, y_delta, b->draw_box.w, olbl->font_em);
  cairo_fill(cr);

  cairo_select_font_face(cr, olbl->font_name, CAIRO_FONT_SLANT_NORMAL,
                                               CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr, olbl->font_size);

  int cdx = 0;
  do {
    cairo_text_extents_t search_extents;
    char *data = ((char**)olbl->string_mete)[cdx];

    cairo_text_extents(cr, data, &search_extents);

    int bx = BUTTON_TEXT_MIN,
        by = 0;
    int whtsp = olbl->draw_box.w - search_extents.x_advance;
    if (whtsp > 0) {
      int position = olbl->state & HJST_MSK;
      if      (position == HJST_LFT) {  whtsp = 0;  }
      else if (position == HJST_CTR) {  whtsp /= 2; }
      bx += whtsp;
    }

    double x = b->draw_box.x,
           y = b->draw_box.y + (cdx * olbl->font_em);
    cairo_set_source_rgba(cr, 0, 0, 0, 1);
    cairo_move_to(cr, x + bx, y + olbl->font_origin + by);
    cairo_show_text(cr, data);
  } while ((++cdx) < 2);

}

static gboolean
popup_draw_event(PhxInterface *pport, cairo_t *cr, void *widget) {

  (void)widget;

     // only 1 object to tport
  PhxObject *obj = pport->object_list[1];
  obj->_draw_cb(obj, cr);
  return TRUE;
}

GtkWidget *main_window = NULL;

static int
popup_meter(PhxInterface *pport, GdkEvent *event, void *widget) {

  if (event->type == GDK_MOTION_NOTIFY) {
    PhxObject *obj = pport->object_list[1];
    obj->state &= ~1;
    obj->state |= (event->motion.y > (obj->mete_box.h / 2));
    cairo_region_t *crr;
    crr = cairo_region_create_rectangle(
             (cairo_rectangle_int_t *)&obj->mete_box);
    gdk_window_invalidate_region(pport->parent_window, crr, FALSE);
    cairo_region_destroy(crr);
    return TRUE;
  }
  if (event->type == GDK_BUTTON_RELEASE) {
    PhxObjectButton *obtn
                    = (PhxObjectButton *)findPort->object_list[choose_box];
    PhxObject *obj = pport->object_list[1];
    _Bool set = obj->state & 1;
    visible_set(findPort->object_list[close0_box], !set);
    visible_set(findPort->object_list[replace_all_box], set);
    visible_set(findPort->object_list[replace_box], set);
    visible_set(findPort->object_list[replace_find_box], set);
    visible_set(findPort->object_list[close1_box], set);
    visible_set(findPort->object_list[textview_replace_box], set);
      // set selected label
    PhxObjectLabel *olbl = obtn->label;
    if (olbl->string != ((char**)olbl->string_mete)[(int)set])
      olbl->string = ((char**)olbl->string_mete)[(int)set];
    if (!set)  lci_findport_search(findPort);
      // set viewable size, one or two rows
    int idx = (BOX_HEIGHT > 20);
    int two_row_height = (BOX_HEIGHT * 2) + window_adjustments[idx][1];
    GtkWindow *window = GTK_WINDOW(main_window);
    if (set) {
      if (findPort->mete_box.h != two_row_height)
        gtk_window_resize(window, findPort->mete_box.w, two_row_height);
    } else {
      if (findPort->mete_box.h == two_row_height)
        gtk_window_resize(window, findPort->mete_box.w, BOX_HEIGHT);
    }
    gtk_widget_destroy(widget);
      // redraw findPort
    cairo_region_t *crr;
    crr = cairo_region_create_rectangle(
             (cairo_rectangle_int_t *)&findPort->mete_box);
    gdk_window_invalidate_region(findPort->parent_window, crr, FALSE);
    cairo_region_destroy(crr);
    return TRUE;
  }
  if (event->type == GDK_FOCUS_CHANGE) {
    if (!event->focus_change.in)  gtk_widget_destroy(widget);
    return TRUE;
  }
  return FALSE;
}

static GtkWidget *
findport_combo_run(LCIFindPort *fport, PhxObjectButton *obtn) {

  GtkWidget *combo_popup;
  combo_popup = gtk_window_new(GTK_WINDOW_POPUP);
  GtkWidget *content = gtk_drawing_area_new();
  gtk_container_add(GTK_CONTAINER(combo_popup), content);

  gtk_widget_realize(combo_popup);
  gtk_widget_realize(content);

  int x = obtn->draw_box.x - obtn->mete_box.x - 1,
      y = obtn->draw_box.y - obtn->mete_box.y - 1,
      w = obtn->draw_box.w - obtn->label->child->draw_box.w,
      h = obtn->draw_box.h * 2;
  gtk_window_set_default_size(GTK_WINDOW(combo_popup), w, h);
  PhxInterface *pport;
    // window uses different coords, use draw_box to move window (below)
  pport = ui_interface_create((GtkDrawingArea*)content, 0, 0, w, h);
  PhxObject *obj = ui_object_create(PHX_POPUP, draw_popup, 0, 0, w, h);
  ui_interface_add(pport, obj);
    // after add so full event box
  ui_box_inset(&obj->draw_box, x, y, x, y);
    // combo menu data
  obj->child = (PhxObject*)obtn->label;

  g_signal_connect_swapped(G_OBJECT(combo_popup), "draw",
                                G_CALLBACK(popup_draw_event), pport);
    // NOTE: can't change cursor unless connect to top-most
  gtk_widget_set_can_focus(combo_popup, TRUE);
  g_signal_connect_swapped(G_OBJECT(combo_popup), "focus-in-event",
                                G_CALLBACK(popup_meter), pport);
  g_signal_connect_swapped(G_OBJECT(combo_popup), "focus-out-event",
                                G_CALLBACK(popup_meter), pport);
  g_signal_connect_swapped(G_OBJECT(combo_popup), "button-press-event",
                                G_CALLBACK(popup_meter), pport);
  g_signal_connect_swapped(G_OBJECT(combo_popup), "button-release-event",
                                G_CALLBACK(popup_meter), pport);
    // connections not auto by gtk
  gtk_widget_add_events(combo_popup, GDK_POINTER_MOTION_MASK);
  g_signal_connect_swapped(G_OBJECT(combo_popup), "motion-notify-event",
                                G_CALLBACK(popup_meter), pport);

    // set position based on active text
  int delta_y = 0;
  PhxObjectLabel *olbl = obtn->label;
  olbl->state &= ~1;
  if (olbl->string != ((char**)olbl->string_mete)[0])
    olbl->state |= 1, delta_y = -olbl->font_em, obj->state |= 1;
  delta_y += obtn->draw_box.y;

  double dx, dy;
  GdkWindow *window = gtk_widget_get_window(combo_popup);
  gdk_window_coords_to_parent(
                       gdk_window_get_parent(obtn->iface->parent_window),
                                     obtn->draw_box.x, delta_y, &dx, &dy);

  gdk_window_move(window, dx, dy);
  gtk_widget_show_all(combo_popup);

  return combo_popup;
}

// with demo passes textview, LCode uses gtk, pass GtkTextBuffer *tbuf instead
static int
findport_display_results(LCIFindPort *fport, struct _fsearch *fdata) {

  int found = 0;
  if (fdata != NULL) {
    int idx = 0;
    do {
      unsigned bits = (fdata->q_results + idx)->qbits;
      bits = bits - ((bits >> 1) & 0x55555555);
      bits = (bits & 0x33333333) + ((bits >> 2) & 0x33333333);
      bits = (bits + (bits >> 4)) & 0x0F0F0F0F;
      bits *= 0x01010101;
      found += bits >> 24;
    } while ((++idx) <= fdata->n_qr);
  }

    // could sprint direct
  char rbuf[32];
  sprintf(rbuf, "%d found matches", found);
  PhxObjectLabel *olbl = (PhxObjectLabel*)fport->object_list[found_box];
  ui_textview_buffer_set(olbl, rbuf);

  _Bool set = found > 1;
  if (found == 1) {
    struct _results *results = &fdata->q_results[(fdata->qdx >> 5)];
    unsigned bits = results->qbits;
    int mnrdx = fdata->qdx & ~(-1 << 5);
    set = !(bits & (0x00000001U << mnrdx));
    if (!set) {
      PhxObjectTextview *otxt = (PhxObjectTextview*)fport->param_data;
      set = (otxt->insert.offset == results->qoffsets[mnrdx]);
    }
  }
  sensitive_set(fport->object_list[navigate_right_box], set);
  sensitive_set(fport->object_list[navigate_left_box], set);

  cairo_region_t *crr;
  crr = cairo_region_create_rectangle(
           (cairo_rectangle_int_t *)&fport->mete_box);
  gdk_window_invalidate_region(fport->parent_window, crr, FALSE);
  cairo_region_destroy(crr);
  return found;
}

static struct _fsearch *
findport_results_for(LCIFindPort *fport, char *filename) {

  struct _fsearch *searches = fport->fsearchs;
  if (searches == NULL) {
    fport->nfsearch = 0;
    fport->fsearchs = (searches = malloc(sizeof(struct _fsearch)));
qfile_create:
    searches->qfile = NULL;
    if (filename != NULL)
      searches->qfile = strdup(filename);
    searches->q_results = malloc(sizeof(struct _results));
    searches->q_results->qbits = 0;
    searches->n_qr = 0;
    searches->qdx = 0;
    searches->changed_id = 0;
    searches->dirtyo = INT_MAX;
    return searches;
  }
  int idx = 0;
  char *fName = filename;
  do {
    char *sName = (&searches[idx])->qfile;
    if ((sName == NULL) && (fName == NULL))  break;
    if ((sName != NULL) && (fName != NULL))
      if (strcmp(sName, fName) == 0)  break;
    if ((++idx) > fport->nfsearch) {
      struct _fsearch *newfs;
      newfs = realloc(fport->fsearchs,
                         ((idx + 1) * sizeof(struct _fsearch)));
      if (newfs == NULL) {
        puts("realloc failed: findport_results_for");
        return NULL;
      }
      fport->nfsearch++;
      fport->fsearchs = newfs;
      searches = &newfs[idx];
      goto qfile_create;
    }
  } while (1);
  return searches;
}

static void
findport_reset_search_data(struct _fsearch *search) {
    // clear tags, then result data
  if (search->search_string != NULL) {
    free(search->search_string);
    search->search_string = NULL;
  }
  if (search->n_qr != 0)
    search->q_results
                = realloc(search->q_results, sizeof(struct _results));
  search->q_results->qbits = 0;
  search->n_qr = 0;
  search->qdx = 0;
}

static int
findport_search_update_from(LCIFindPort *fport, int start) {

  PhxObjectTextview *textview = (PhxObjectTextview*)fport->param_data;
  if (textview == NULL)  return -1;

  if (textview->dirtyo < start)  start = textview->dirtyo;
#if USE_MARKS
  text_marks_update(textview);
#endif
  textview->dirtyo = INT_MAX;
  text_buffer_reset(textview);

  struct _fsearch *searches = findport_results_for(fport, NULL);
  if (searches == NULL)  return -1;
  if (searches->search_string == NULL)  return -1;

  struct _results *results;
  int mnrdx, majdx, offset;
  majdx = searches->qdx >> 5;
  mnrdx = searches->qdx & ~(1U << 5);
  offset = (searches->q_results + majdx)->qoffsets[mnrdx];
    // set to unknown if destroying
  if (offset > start)
    searches->qdx = 0;

  majdx = searches->n_qr;
  results = searches->q_results + majdx;
    // reguardless if valid, use 0 mnrdx
  offset = results->qoffsets[0];
  while ((offset > start) && (majdx > 0)) {
    results = searches->q_results + (--majdx);
    offset = results->qoffsets[0];
  }
  if (majdx < searches->n_qr) {
    searches->n_qr = majdx;
    struct _results *newPtr;
    newPtr = realloc(searches->q_results,
                      ((searches->n_qr + 1) * sizeof(struct _results)));
    if (newPtr == NULL)  return -1;
    searches->q_results = newPtr;
    results = searches->q_results + searches->n_qr;
  }
  if (searches->n_qr == 0)
    offset = 0;

  mnrdx = 0;
  char *key = searches->search_string;
  size_t key_len = strlen(key);
  char *data = textview->string;
  char *rdPtr = textview->string + offset;
  if ((rdPtr = strstr(rdPtr, key)) != NULL) {
    do {
      results->qoffsets[mnrdx] = rdPtr - data;
      if ((rdPtr = strstr((rdPtr + key_len), key)) == NULL)  break;
      if ((++mnrdx) == 32) {
        results->qbits = 0xFFFFFFFFU;
        searches->n_qr++;
        struct _results *newPtr;
        newPtr = realloc(searches->q_results,
                          ((searches->n_qr + 1) * sizeof(struct _results)));
        if (newPtr == NULL)  return -1;
        searches->q_results = newPtr;
        results = searches->q_results + searches->n_qr;
        mnrdx = 0;
      }
    } while (1);
    results->qbits = 0xFFFFFFFFU >> (31 - mnrdx);
  }
  return findport_display_results(fport, searches);
}

static unsigned
lsbDeBruijn32(unsigned v) {

  static const unsigned lsbDeBruijn[32] = {
    0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
    31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
  };
  return lsbDeBruijn[((unsigned)((v & -v) * 0x077CB531U)) >> 27];
}

static unsigned
msbDeBruijn32(unsigned v) {

  static const unsigned msbDeBruijn[32] = {
    0, 9, 1, 10, 13, 21, 2, 29, 11, 14, 16, 18, 22, 25, 3, 30,
    8, 12, 20, 28, 15, 17, 24, 7, 19, 27, 23, 6, 26, 5, 4, 31
  };

  v |= v >> 1; // first round down to one less than a power of 2
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;

  return msbDeBruijn[(unsigned)(v * 0x07C4ACDDU) >> 27];
}

static int
_qresult_maximum(struct _fsearch *searches) {

  int majdx = searches->n_qr;
  struct _results *results = searches->q_results + majdx;
  while (results->qbits == 0) {
      // case: empty set of bits in searches, entered with empty
    if ((--majdx) < 0)  return -1;
    results = searches->q_results + majdx;
  }
  return ((majdx << 5) | msbDeBruijn32(results->qbits));
}

// note: should have different negative error codes
// note: should be range check, alteration may have occured on part of found
static int
findport_update_position(LCIFindPort *fport, int result_direction) {

  PhxObjectTextview *receiver;
  receiver = (PhxObjectTextview*)fport->object_list[textview_find_box];
  size_t rSz = receiver->str_nil;

  PhxObjectTextview *otxt = (PhxObjectTextview *)fport->param_data;
  int ins = otxt->insert.offset;

  struct _fsearch *searches = findport_results_for(fport, NULL);
  int offset = _qresult_maximum(searches);
  if (offset < 0)  return -1;
  int majdx = offset >> 5;
  int mnrdx = offset & ~(0xFFFFFFFFU << 5);
  struct _results *results = searches->q_results + majdx;
  offset = results->qoffsets[mnrdx];

  int ret_offset = offset;
  int ret_majdx = majdx,
      ret_mnrdx = mnrdx;

  if (offset <= ins) {
    _Bool selected = (offset == ins) && (otxt->release.offset == (ins + rSz));
    if ( (result_direction == RESULT_LEFT)
         && ( (offset < ins) || ( (offset == ins) && !selected ) ) )
      goto mo_ret;
    if ((offset < ins) || selected) {
      if (result_direction == RESULT_RIGHT) {
          // set exit as min offset result
        ret_majdx = 0;
        results = searches->q_results;
        while (results->qbits == 0)
          results = searches->q_results + (++ret_majdx);
        ret_mnrdx = lsbDeBruijn32(results->qbits);
        ret_offset = results->qoffsets[ret_mnrdx];
        goto mo_ret;
      }
    }
  }
    // note: doesn't care if alters removed offset values
  unsigned bits = results->qbits;
  do {
    if ((bits & (0x00000001U << mnrdx)) != 0) {
      ret_offset = offset;
      ret_majdx = majdx,
      ret_mnrdx = mnrdx;
    }
    if ((--mnrdx) < 0) {
      do {
        if ((--majdx) < 0) {
          if (result_direction == RESULT_LEFT) {
            offset = _qresult_maximum(searches);
            searches->qdx = offset;
            majdx = offset >> 5;
            mnrdx = offset & ~(0xFFFFFFFFU << 5);
            offset = (searches->q_results + majdx)->qoffsets[mnrdx];
            return offset;
          }
          goto mo_ret;
        }
        results = searches->q_results + majdx;
      } while ((bits = results->qbits) == 0);
      mnrdx = msbDeBruijn32(bits);
    }
    offset = results->qoffsets[mnrdx];
  } while (offset > ins);
  if ( (offset == ins) && (result_direction == RESULT_RIGHT)
      && (otxt->release.offset != (ins + rSz)) ) {
    ret_offset = offset;
    ret_majdx = majdx,
    ret_mnrdx = mnrdx;
  }
mo_ret:
  if (result_direction == RESULT_LEFT) {
    if (offset == ins) {
      results = searches->q_results + majdx;
      if (((bits = results->qbits) & (0x7FFFFFFF >> (31 - mnrdx))) == 0)
         goto refill;
      do {
        if ((--mnrdx) < 0) {
          do {
      refill:
            if ((--majdx) < 0)  majdx = searches->n_qr;
            results = searches->q_results + majdx;
          } while ((bits = results->qbits) == 0);
          mnrdx = msbDeBruijn32(bits);
          break;
        }
      } while ((bits & (0x00000001U << mnrdx)) == 0);
    }
    ret_offset = results->qoffsets[mnrdx];
    ret_majdx = majdx;
    ret_mnrdx = mnrdx;
  }
  offset = ret_offset;
  majdx = ret_majdx;
  mnrdx = ret_mnrdx;
  searches->qdx = (majdx << 5) + mnrdx;
  return offset;
}

static void
findport_navigate_left(LCIFindPort *fport) {

  PhxObjectTextview *otxt = (PhxObjectTextview *)fport->param_data;
  text_buffer_reset(otxt);
  findport_update_position(fport, RESULT_LEFT);
  text_buffer_apply_search_tag(fport, findport_results_for(fport, NULL));
}

static void
findport_navigate_right(LCIFindPort *fport) {

  PhxObjectTextview *otxt = (PhxObjectTextview *)fport->param_data;
  text_buffer_reset(otxt);
  findport_update_position(fport, RESULT_RIGHT);
  text_buffer_apply_search_tag(fport, findport_results_for(fport, NULL));
}

static void
findport_replace_all(LCIFindPort *fport) {

  PhxObjectTextview *otxt = (PhxObjectTextview *)fport->param_data;
  text_buffer_reset(otxt);

  struct _fsearch *searches = findport_results_for(fport, NULL);
  if (searches == NULL)  return;
  if (searches->search_string == NULL)  return;
  size_t rSz = strlen(searches->search_string);

  PhxObjectTextview *sender;
  sender = (PhxObjectTextview*)fport->object_list[textview_replace_box];
  text_buffer_reset(sender);
    // delta of replacements
  int delta = sender->str_nil - rSz;
  if ((delta == 0) && (strcmp(searches->search_string, sender->string) == 0))
    return;

    // create an editable buffer, non-drawing
  PhxObjectTextview *buffered;
  buffered = (PhxObjectTextview*)ui_object_create(PHX_TEXTBUFFER,
                                                         NULL, 0, 0, 0, 0);
  ui_textview_buffer_set(buffered, otxt->string);

  int mnrdx, majdx = searches->n_qr;
  struct _results *results = searches->q_results + majdx;
  unsigned bits = results->qbits;
  while (bits == 0) {
    if ((--majdx) < 0)  // assumes searches was newly created
      goto clean_up;
    results = searches->q_results + majdx;
    bits = results->qbits;
  }
  mnrdx = msbDeBruijn32(bits);
    // for first one only, verify search results is the correct replacable
  int offset = results->qoffsets[mnrdx];
  buffered->insert.offset = offset;
  buffered->release.offset = offset + rSz;
  if (memcmp(&buffered->string[offset], searches->search_string, rSz) != 0) {
clean_up:
  #if USE_MARKS
    free(buffered->newline_list);
  #endif
    free(buffered->string_mete);
    free(buffered);
    return;
  }
  int save_ins_offset = otxt->insert.offset,
      save_rel_offset = otxt->release.offset;
  do {
    int sndx;
      // need to 'catch' amount locations moved by
    if (offset <= save_ins_offset) {
      if (offset == save_ins_offset) {
        save_ins_offset += sender->str_nil;
        save_rel_offset = save_ins_offset;
      } else {
        save_ins_offset += delta;
        save_rel_offset += delta;
      }
    }
    sndx = mnrdx;
    text_buffer_insert(buffered, sender->string, sender->str_nil);
    if ((--mnrdx) < 0) {
      int sjdx;
refill:
      sjdx = majdx;
      do {
        if (majdx == 0) {
          results = searches->q_results + sjdx;
          mnrdx = sndx;
          goto finally;
        }
        results = searches->q_results + (--majdx);
      } while ((bits = results->qbits) == 0);
      mnrdx = msbDeBruijn32(bits);
    }
    unsigned msk = 0x00000001U << mnrdx;
    while ((bits & msk) == 0) {
      if ( ((bits & (0xFFFFFFFF >> (31 - mnrdx))) == 0)
          || ((msk >>= 1) == 0) ) {
        goto refill;
      }
      --mnrdx;
    }
    offset = results->qoffsets[mnrdx];
    buffered->insert.offset = offset;
    buffered->release.offset = offset + rSz;
  } while (1);
finally:
  findport_reset_search_data(searches);
  text_buffer_reset(buffered);
    // update locations
  text_buffer_replace(otxt, buffered->string, buffered->str_nil);
  otxt->insert.offset = save_ins_offset;
  location_for_offset(otxt, &otxt->insert);
  otxt->interim.x = otxt->insert.x;
  otxt->interim.y = otxt->insert.y;
  otxt->interim.offset = otxt->insert.offset;
  location_auto_scroll(otxt, &otxt->insert);
  otxt->release.offset = save_rel_offset;
  location_for_offset(otxt, &otxt->release);
    // side order of redraw
  cairo_region_t *crr;
  crr = cairo_region_create_rectangle(
           (cairo_rectangle_int_t *)&otxt->draw_box);
  gdk_window_invalidate_region(otxt->iface->parent_window, crr, FALSE);
  cairo_region_destroy(crr);
    // buffered is history
#if USE_MARKS
  free(buffered->newline_list);
#endif
  free(buffered->string_mete);
  free(buffered);
}

static void
findport_replace(LCIFindPort *fport) {

  PhxObjectTextview *otxt = (PhxObjectTextview *)fport->param_data;
  if (otxt->type != PHX_TEXTVIEW)  return;
  if (otxt->release.offset != otxt->insert.offset) {
    PhxObjectTextview *receiver;
    size_t tSz = otxt->release.offset - otxt->insert.offset;
    receiver = (PhxObjectTextview*)fport->object_list[textview_find_box];
    text_buffer_reset(receiver);
      // verify selected text is actual 'find' text
    if ( (memcmp(&otxt->string[otxt->insert.offset], receiver->string, tSz) == 0)
        && (receiver->string[tSz] == 0) ) {
      PhxObjectTextview *sender;
      sender = (PhxObjectTextview*)fport->object_list[textview_replace_box];
      text_buffer_reset(sender);

      text_buffer_insert(otxt, sender->string, sender->str_nil);
      findport_search_update_from(fport, otxt->insert.offset);
      findport_update_position(fport, RESULT_RIGHT);
      findport_display_results(fport, findport_results_for(fport, NULL));

      cairo_region_t *crr;
      crr = cairo_region_create_rectangle(
               (cairo_rectangle_int_t *)&otxt->draw_box);
      gdk_window_invalidate_region(otxt->iface->parent_window, crr, FALSE);
      cairo_region_destroy(crr);
    }
  }
}

static void
findport_replace_and_find(LCIFindPort *fport) {

  PhxObjectTextview *otxt = (PhxObjectTextview *)fport->param_data;
  if (otxt->release.offset != otxt->insert.offset) {
    PhxObjectTextview *receiver;
    size_t tSz = otxt->release.offset - otxt->insert.offset;
    receiver = (PhxObjectTextview*)fport->object_list[textview_find_box];
      // verify selected text is actual 'find' text
    if ( (memcmp(&otxt->string[otxt->insert.offset], receiver->string, tSz) == 0)
        && (receiver->string[tSz] == 0) ) {
      PhxObjectTextview *sender;
      sender = (PhxObjectTextview*)fport->object_list[textview_replace_box];
      text_buffer_reset(sender);

      text_buffer_insert(otxt, sender->string, sender->str_nil);
      lci_findport_search(fport);

      cairo_region_t *crr;
      crr = cairo_region_create_rectangle(
               (cairo_rectangle_int_t *)&otxt->draw_box);
      gdk_window_invalidate_region(otxt->iface->parent_window, crr, FALSE);
      cairo_region_destroy(crr);
    }
  }
}

/* demo/design/test has only 1 file, and no attribute set up as of yet */
/* demo also does not use gtk textbuffer ether */
void
lci_findport_clear_results(LCIFindPort *fport) {

  struct _fsearch *searches = fport->fsearchs;
  if (searches == NULL)  return;

  if (searches->qfile != NULL)
    free(searches->qfile);
  if (searches->search_string != NULL)
    free(searches->search_string);
  free(searches->q_results);
  free(searches);
  fport->nfsearch = 0;
  fport->fsearchs = NULL;
}

void
lci_findport_search(LCIFindPort *fport) {

  PhxObjectTextview *textview = (PhxObjectTextview*)fport->param_data;
  if (textview == NULL)  return;
  text_buffer_reset(textview);

    // get storage for results for this textview file
  struct _fsearch *searches = findport_results_for(fport, NULL);
  if (searches == NULL)  return;

  PhxObjectTextview *key_object
           = (PhxObjectTextview*)fport->object_list[textview_find_box];
  if (key_object->str_nil == 0)  return;
  text_buffer_reset(key_object);

  searches->search_string = strdup(key_object->string);
  int found = findport_search_update_from(fport, 0);
  if (found < 0)
    puts("failed error: findport_search_update_from");
  if (found <= 0)  return;

  struct _results *results;
  int ins = textview->insert.offset;
  int mnrdx, majdx = searches->n_qr;
  results = searches->q_results + majdx;
  int offset = results->qoffsets[0];
  while ((offset > ins) && (majdx > 0)) {
    results = searches->q_results + (--majdx);
    offset = results->qoffsets[0];
  }

  int ret_majdx = majdx,
      ret_mnrdx = (mnrdx = 0);

  if (offset < ins) {
    mnrdx = msbDeBruijn32(results->qbits);
    offset = results->qoffsets[mnrdx];
  }
  if (offset <= ins)  goto apply_tag;
  do {
    ret_majdx = majdx;
    ret_mnrdx = mnrdx;
    if ((--mnrdx) < 0) {
      if (majdx == 0)  goto apply_tag;
      results = searches->q_results + (--majdx);
      mnrdx = msbDeBruijn32(results->qbits);
    }
    offset = results->qoffsets[mnrdx];
  } while (offset > ins);
  if (offset == ins) {
    ret_majdx = majdx;
    ret_mnrdx = mnrdx;
  }
apply_tag:
  searches->qdx = (ret_majdx << 5) + ret_mnrdx;
  text_buffer_apply_search_tag(fport, searches);
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
    receiver = (PhxObjectTextview*)findPort->object_list[idx];
    text_buffer_board_set(receiver, data, sz);
  }
}

#pragma mark *** Events ***

static gboolean
configure_event_txtwnd(PhxInterface *iface, GdkEvent *event, void *widget) {

  (void)widget;
  int w_delta = event->configure.width - iface->mete_box.w;
  int h_delta = event->configure.height - iface->mete_box.h;
  iface->mete_box.w = event->configure.width;
  iface->mete_box.h = event->configure.height;

  if ((w_delta != 0) || (h_delta != 0)) {

    PhxObject *obj = iface->object_list[0];
    ui_box_inset(&obj->mete_box, 0, 0, -w_delta, -h_delta);
    ui_box_inset(&obj->draw_box, 0, 0, -w_delta, -h_delta);
    obj = iface->object_list[1];
    ui_box_inset(&obj->mete_box, 0, 0, -w_delta, -h_delta);
    ui_box_inset(&obj->draw_box, 0, 0, -w_delta, -h_delta);
    ui_box_inset(&((PhxObjectTextview*)obj)->bin, 0, 0, -w_delta, -h_delta);

    memset(iface->event_map, 1, iface->map_size);
  }
    /* MUST return FALSE, can not update events pass orginal clip.
       Can draw correctly, but useless without handling events.
       Only thing I didn't try is connect_after_swapped, doesn't exist */
  return FALSE;
}

static gboolean
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
                         = (PhxObjectDrawing*)fport->object_list[0];
    ui_box_inset(&odrw->mete_box, 0, 0, -w_delta, 0);
    ui_box_inset(&odrw->draw_box, 0, 0, -w_delta, 0);
      // alter textview variables, move 'Done' x postion
      // since offseting, change right to opposite left
    PhxObjectButton *obtn = (PhxObjectButton*)fport->object_list[close0_box];
    ui_box_inset(&obtn->mete_box, w_delta, 0, -w_delta, 0);
    ui_box_inset(&obtn->draw_box, w_delta, 0, -w_delta, 0);
    ui_box_inset(&obtn->label->mete_box, w_delta, 0, -w_delta, 0);
    ui_box_inset(&obtn->label->draw_box, w_delta, 0, -w_delta, 0);
    obtn = (PhxObjectButton*)fport->object_list[close1_box];
    ui_box_inset(&obtn->mete_box, w_delta, 0, -w_delta, 0);
    ui_box_inset(&obtn->draw_box, w_delta, 0, -w_delta, 0);
    ui_box_inset(&obtn->label->mete_box, w_delta, 0, -w_delta, 0);
    ui_box_inset(&obtn->label->draw_box, w_delta, 0, -w_delta, 0);

    PhxObjectTextview *otxt
                = (PhxObjectTextview*)fport->object_list[textview_find_box];
    ui_box_inset(&otxt->mete_box, 0, 0, -w_delta, 0);
    ui_box_inset(&otxt->draw_box, 0, 0, -w_delta, 0);
    ui_box_inset(&otxt->bin, 0, 0, -w_delta, 0);

    otxt = (PhxObjectTextview*)fport->object_list[textview_replace_box];
    ui_box_inset(&otxt->mete_box, 0, 0, -w_delta, 0);
    ui_box_inset(&otxt->draw_box, 0, 0, -w_delta, 0);
    ui_box_inset(&otxt->bin, 0, 0, -w_delta, 0);
  }
  fport->mete_box.w = width;

  int height = event->configure.height;
  int h_delta = height - fport->mete_box.h;
  if (fport->mete_box.h != height) {
    PhxObjectDrawing *odrw
                         = (PhxObjectDrawing*)fport->object_list[0];
    ui_box_inset(&odrw->mete_box, 0, 0, 0, h_delta);
    ui_box_inset(&odrw->draw_box, 0, 0, 0, h_delta);
  }
  fport->mete_box.h = height;

  if ( (w_delta != 0) || (h_delta != 0) )
    ui_interface_refresh(fport);

    /* MUST return FALSE, can not update events pass orginal clip.
       Can draw correctly, but useless without handling events.
       Only thing I didn't try is connect_after_swapped, doesn't exist.
       Only a guess, but maybe has something to do with containers?
       A window forces one to connect any drawing to a container.
       User doesn't resize container, maybe during expand, events
       not expanded unless one figures odd connection. */
  return FALSE;
}

static _Bool
mouse_drag_begin(PhxInterface *iface, GdkEvent *event) {

  if ((event->button.state & GDK_CONTROL_MASK) != 0) {
    ui_interface_mouse_set(iface, DND_COPY_CURSOR);
    ui_interface_set_cursor_named(iface->parent_window, "copy");
  } else {
    ui_interface_mouse_set(iface, DND_MOVE_CURSOR);
    ui_interface_set_cursor_named(iface->parent_window, "move");
  }
  return TRUE;
}

static _Bool
mouse_drag_motion(PhxInterface *iface, GdkEvent *event) {

  PhxObject *obj;
  _Bool      in_mete = TRUE;

  int x = (int)(event->motion.x);
  int y = (int)(event->motion.y);
  if ( ((unsigned)x <= (unsigned)iface->mete_box.w)
      && ((unsigned)y <= (unsigned)iface->mete_box.h)) {
    int ldx = iface->event_map[x][y];
    obj = iface->object_list[ldx];
  } else {
// get window at pointer, querry if can accept
// for now use iface focus to handle event
    obj = iface->has_focus;
    in_mete = FALSE;
  }

  double this_event_x = event->motion.x,
         this_event_y = event->motion.y;

  if ( (fabs(this_event_x - press_event_x) > 1.0)
      || (fabs(this_event_y - press_event_y) > 1.0) ) {

    press_event_x = this_event_x, press_event_y = this_event_y;

    if ((obj->type == PHX_TEXTVIEW) || (obj->type == PHX_ENTRY)) {

      PhxObjectTextview *tv = (PhxObjectTextview*)obj;

      location temp;
      temp.x = (int)event->motion.x - tv->draw_box.x;
      temp.y = (int)event->motion.y - tv->draw_box.y;
      if (temp.x < 0)  temp.x = -1;
      if (temp.y < 0)  temp.y = -1;
      temp.x += tv->bin.x;
      temp.y += tv->bin.y;
      location_for_point(tv, &temp);
      location_auto_scroll(tv, &temp);
      if (!in_mete)  goto outside;
      if ( (temp.offset == tv->release.offset)
          || (temp.offset == tv->insert.offset) ) {

        if ( (temp.offset != tv->drop.offset)
            || (ui_interface_mouse_get(iface) == DND_COPY_CURSOR) ) {
          tv->drop.x = temp.x;
          tv->drop.y = temp.y;
          tv->drop.offset = temp.offset;
          goto redraw;
        }
      } else if ( (temp.offset > tv->release.offset)
          || (temp.offset < tv->insert.offset) ) {
        if (temp.offset != tv->drop.offset) {
          tv->drop.x = temp.x;
          tv->drop.y = temp.y;
          tv->drop.offset = temp.offset;
          goto redraw;
        }
      } else {
 outside:
        tv->drop.x = tv->interim.x;
        tv->drop.y = tv->interim.y;
        tv->drop.offset = tv->interim.offset;

        cairo_region_t *crr;
  redraw:
        crr = cairo_region_create_rectangle(
                 (cairo_rectangle_int_t *)&iface->mete_box);
        gdk_window_invalidate_region(event->motion.window, crr, FALSE);
        cairo_region_destroy(crr);
      }
    }
  }
  return TRUE;
}

static _Bool
mouse_drag_finish(PhxInterface *iface, GdkEvent *event) {

  text_buffer_drag_release((PhxObjectTextview*)iface->has_focus);

  ui_interface_mouse_set(iface, MOUSE_CLEAR);
  ui_interface_set_cursor_named(iface->parent_window, "text");

  cairo_region_t *crr;
  crr = cairo_region_create_rectangle(
           (cairo_rectangle_int_t *)&iface->mete_box);
  gdk_window_invalidate_region(event->button.window, crr, FALSE);
  cairo_region_destroy(crr);

  return TRUE;
}

static void
mouse_shift_click(PhxObjectTextview *tv, location *temp) {

  location *active;
  int font_em = tv->font_em;

    // special handle y for mouse_motion, esp. when one line view
  if (ui_interface_mouse_get(tv->iface) == MOUSE_1BUTTON_MOTION) {
    if ((temp->y - 2) <= tv->bin.y) {
      if ( (tv->bin.y != 0)
          && ((tv->bin.y - ((tv->bin.y / font_em) * font_em)) == 0) )
      temp->y -= font_em;
    } else if ((temp->y + 2) >= tv->bin.h) {
      location end;
      end.offset = (int)strlen(tv->string);
      location_for_offset(tv, &end);
        // note 'equal to' means last line not shown
      if (tv->bin.h < (end.y + font_em))
        temp->y += font_em;
    }
  }
  location_for_point(tv, temp);
  location_auto_scroll(tv, temp);

  active = &tv->release;
  if (temp->offset < tv->release.offset) {
    active = &tv->insert;
    if ( (temp->offset > tv->insert.offset)
        && (tv->release.offset == tv->interim.offset) )
      active = &tv->release;
  }
  if (active->offset != temp->offset) {
    tv->interim.x = (active->x = temp->x);
    tv->interim.y = (active->y = temp->y);
    tv->interim.offset = (active->offset = temp->offset);
  }
}

// will enter either mouse_shift_click or mouse_press_event
// use inerim as start point (last active)
static void
mouse_double_click(PhxObjectTextview *tv, int shift_click) {

  location *temp;
  int offset;
  char find_ch;
  char ch0, *cPtr, *iPtr = &tv->string[tv->interim.offset];
  ch0 = *(cPtr = iPtr);

  if (ch0 == 0)  return;

  if (isspace(ch0) != 0) {
      // moves right, at least 1 found
    find_ch = ch0;
    do  ch0 = *(++cPtr);  while (find_ch == ch0);
    offset = (unsigned)(cPtr - tv->string);
    if ( (shift_click && (offset > tv->release.offset))
       || !shift_click ) {
      tv->release.offset = offset;
      location_for_offset(tv, &tv->release);
      temp = &tv->release;
    } else if (shift_click && (offset == tv->release.offset))
      temp = &tv->release;
    if (tv->interim.offset != 0) {
      cPtr = iPtr;
      while ( ((cPtr - 1) >= tv->string)
             && (*(cPtr - 1) == find_ch) )
        --cPtr;
      offset = (unsigned)(cPtr - tv->string);
      if ( (shift_click && (offset < tv->insert.offset))
         || !shift_click ) {
        tv->insert.offset = offset;
        location_for_offset(tv, &tv->insert);
        temp = &tv->insert;
      } else if (shift_click && (offset == tv->insert.offset))
        temp = &tv->insert;
    }
    tv->interim.x = temp->x;
    tv->interim.y = temp->y;
    tv->interim.offset = temp->offset;
    location_auto_scroll(tv, temp);
    return;
  }

  if ((isalnum(ch0) != 0) || (ch0 == '_')) {
    do
      ch0 = *(++cPtr);
    while ((isalnum(ch0) != 0) || (ch0 == '_'));
    offset = (unsigned)(cPtr - tv->string);
    if ( (shift_click && (offset > tv->release.offset))
       || !shift_click ) {
      tv->release.offset = offset;
      location_for_offset(tv, &tv->release);
      temp = &tv->release;
    } else if (shift_click && (offset == tv->release.offset))
      temp = &tv->release;
    if (tv->insert.offset != 0) {
      cPtr = iPtr;
      while ((cPtr - 1) >= tv->string) {
        ch0 = *(cPtr - 1);
        if ((isalnum(ch0) == 0) && (ch0 != '_'))  break;
        --cPtr;
      }
      offset = (unsigned)(cPtr - tv->string);
      if ( (shift_click && (offset < tv->insert.offset))
         || !shift_click ) {
        tv->insert.offset = offset;
        location_for_offset(tv, &tv->insert);
        temp = &tv->insert;
      } else if (shift_click && (offset == tv->insert.offset))
        temp = &tv->insert;
    }
    tv->interim.x = temp->x;
    tv->interim.y = temp->y;
    tv->interim.offset = temp->offset;
    location_auto_scroll(tv, temp);
    return;
  }

// c_parsing, before punct, since punct based

  if (ispunct(ch0) != 0) {
      // moves right, at least 1 found
    do  ch0 = *(++cPtr);  while (ispunct(ch0) != 0);
    offset = (unsigned)(cPtr - tv->string);
    if ( (shift_click && (offset > tv->release.offset))
       || !shift_click ) {
      tv->release.offset = offset;
      location_for_offset(tv, &tv->release);
      temp = &tv->release;
    } else if (shift_click && (offset == tv->release.offset))
      temp = &tv->release;
    if (tv->interim.offset != 0) {
      cPtr = iPtr;
      while ((cPtr - 1) >= tv->string) {
        ch0 = *(cPtr - 1);
        if (ispunct(ch0) == 0)  break;
        --cPtr;
      }
      offset = (unsigned)(cPtr - tv->string);
      if ( (shift_click && (offset < tv->insert.offset))
         || !shift_click ) {
        tv->insert.offset = offset;
        location_for_offset(tv, &tv->insert);
        temp = &tv->insert;
      } else if (shift_click && (offset == tv->insert.offset))
        temp = &tv->insert;
    }
    tv->interim.x = temp->x;
    tv->interim.y = temp->y;
    tv->interim.offset = temp->offset;
    location_auto_scroll(tv, temp);
    return;
  }
}

// will enter either mouse_shift_click or mouse_press_event
// use inerim as start point (last active)
static void
mouse_triple_click(PhxObjectTextview *tv, int shift_click) {

  location *temp;

  char *tempPtr = strchr(&tv->string[tv->interim.offset], '\n');
  int offset = (int)((tempPtr + 1) - tv->string);
  if (tempPtr == NULL) offset = (int)strlen(tv->string);
  if ( (shift_click && (offset > tv->release.offset))
      || !shift_click ) {
    tv->release.offset = offset;
    location_for_offset(tv, &tv->release);
    temp = &tv->release;
  }
  if (tv->interim.offset != 0) {
    tempPtr = memrchr(tv->string, '\n', tv->interim.offset);
    tempPtr = (tempPtr == NULL) ? tv->string : (tempPtr + 1);
    offset = (int)(tempPtr - tv->string);
    if ( (shift_click && (offset < tv->insert.offset))
        || !shift_click ) {
      tv->insert.offset = offset;
      location_for_offset(tv, &tv->insert);
      temp = &tv->insert;
    }
  }
  tv->interim.x = temp->x;
  tv->interim.y = temp->y;
  tv->interim.offset = temp->offset;
}

static _Bool
mouse_press_event_txt(PhxInterface *iface, GdkEvent *event, PhxObject *obj) {

  if ((obj->type != PHX_TEXTVIEW) && (obj->type != PHX_ENTRY))
    return FALSE;

    // first 'click' only sets focus
  if (obj != iface->has_focus) {
    iface->has_focus = obj;
      // at min, draw text cursor
    goto redraw;
  }

  ui_interface_mouse_set(iface, MOUSE_CLEAR);

  PhxObjectTextview *tv = (PhxObjectTextview*)obj;

    // flatten text_buffer, if was editing
  text_buffer_reset(tv);

  int shift_click = ((event->button.state & GDK_SHIFT_MASK) != 0);
  if (event->button.type == GDK_2BUTTON_PRESS) {
    mouse_double_click(tv, shift_click);
    goto redraw;
  }
  if (event->button.type == GDK_3BUTTON_PRESS) {
    mouse_triple_click(tv, shift_click);
    goto redraw;
  }
  if (event->button.type == GDK_BUTTON_PRESS)
    ui_interface_mouse_set(iface, MOUSE_1BUTTON);

  if (shift_click) {
      // Do nothing until release event, or motion
    return TRUE;
  }

  press_event_x = event->button.x;
  press_event_y = event->button.y;

  tv->interim.x = (int)event->button.x - tv->draw_box.x;
  tv->interim.y = (int)event->button.y - tv->draw_box.y;
  if (tv->interim.x < 0)  tv->interim.x = 0;
  if (tv->interim.y < 0)  tv->interim.y = 0;
  tv->interim.x += tv->bin.x;
  tv->interim.y += tv->bin.y;

  location_for_point(tv, &tv->interim);
  location_auto_scroll(tv, &tv->interim);

    // if clicked in selection do nothing until motion or release
  if ( (tv->insert.offset == tv->release.offset)
       || ( ((tv->interim.offset < tv->insert.offset)
          && (tv->interim.offset < tv->release.offset))
        || ((tv->interim.offset > tv->insert.offset)
            && (tv->interim.offset > tv->release.offset)) ) ) {
    tv->insert.x = (tv->release.x = tv->interim.x);
    tv->insert.y = (tv->release.y = tv->interim.y);
    tv->insert.offset = (tv->release.offset = tv->interim.offset);
  }
    // update even if no size change, prevents black pixel(s) line at bottom
  cairo_region_t *crr;
redraw:
  crr = cairo_region_create_rectangle(
           (cairo_rectangle_int_t *)&iface->mete_box);
  gdk_window_invalidate_region(event->button.window, crr, FALSE);
  cairo_region_destroy(crr);
  return TRUE;
}

static _Bool
mouse_release_event_txt(PhxInterface *iface, GdkEvent *event, PhxObject *obj) {

  int x = (int)(event->button.x);
  int y = (int)(event->button.y);

  if (ui_interface_mouse_get(iface) & MOUSE_1BUTTON) {

    PhxObjectTextview *tv = (PhxObjectTextview*)iface->has_focus;

    if ((event->button.state & GDK_SHIFT_MASK) != 0) {

      location temp;
      temp.x = x - tv->draw_box.x;
      temp.y = y - tv->draw_box.y;
      if (temp.x < 0)  temp.x = 0;
      if (temp.y < 0)  temp.y = 0;
      temp.x += tv->bin.x;
      temp.y += tv->bin.y;

      mouse_shift_click(tv, &temp);
      goto redraw;
    }
      // use only if !selection (pressed-released, no motion)
    if ((ui_interface_mouse_get(iface) & MOUSE_MOTION) == 0) {
      tv->insert.x = (tv->release.x = tv->interim.x);
      tv->insert.y = (tv->release.y = tv->interim.y);
      tv->insert.offset = (tv->release.offset = tv->interim.offset);
    } else {
        // motion-released, set cursor based on release point
      GdkWindow *window = iface->parent_window;
      GdkCursor *cursor = gdk_window_get_cursor(window);
      if ( (window == event->button.window)
          && ((obj->type == PHX_TEXTVIEW) || (obj->type == PHX_ENTRY)) ) {
        if (cursor == NULL) {
          GdkDisplay *display = gdk_window_get_display(window);
          cursor = gdk_cursor_new_from_name(display, "text");
          gdk_window_set_cursor(window, cursor);
        }
      } else {
        if (cursor != NULL) {
          gdk_window_set_cursor(window, NULL);
          g_object_unref((void*)cursor);
        }
      }
    }
  }
redraw:
  ui_interface_mouse_set(iface, MOUSE_CLEAR);
  cairo_region_t *crr;
  crr = cairo_region_create_rectangle(
           (cairo_rectangle_int_t *)&iface->mete_box);
  gdk_window_invalidate_region(event->button.window, crr, FALSE);
  cairo_region_destroy(crr);
  return TRUE;
}

static _Bool
mouse_motion_event_txt(PhxInterface *iface, GdkEvent *event, PhxObject *obj) {

  if (ui_interface_mouse_get(iface) == MOUSE_CLEAR)  return FALSE;

  double this_event_x = event->button.x,
         this_event_y = event->button.y;

  if ( (fabs(this_event_x - press_event_x) > 1.0)
      || (fabs(this_event_y - press_event_y) > 1.0) ) {

    press_event_x = this_event_x, press_event_y = this_event_y;

    PhxObjectTextview *tv = (PhxObjectTextview*)iface->has_focus;

// if clicked in a selection, and was first MOUSE_1BUTTON_MOTION
// then start drag
    if (ui_interface_mouse_get(iface) == MOUSE_1BUTTON) {
      if (tv->insert.offset != tv->release.offset) {
        if ( ((tv->interim.offset > tv->insert.offset)
              && (tv->interim.offset < tv->release.offset))
            || ((tv->interim.offset < tv->insert.offset)
                && (tv->interim.offset > tv->release.offset)) ) {
          tv->drop.x = tv->interim.x;
          tv->drop.y = tv->interim.y;
          tv->drop.offset = tv->interim.offset;
          return mouse_drag_begin(iface, event);
        } else {
            // clear selection
          tv->insert.x = (tv->release.x = tv->interim.x);
          tv->insert.y = (tv->release.y = tv->interim.y);
          tv->insert.offset = (tv->release.offset = tv->interim.offset);
        }
      }
    }

    location temp;
    temp.x = (int)this_event_x - tv->draw_box.x;
    temp.y = (int)this_event_y - tv->draw_box.y;
    if (temp.x < 0)  temp.x = 0;
    if (temp.y < 0)  temp.y = 0;
    temp.x += tv->bin.x;
    temp.y += tv->bin.y;

    ui_interface_mouse_set(iface, MOUSE_CLEAR);
      // force no use of mouse_release on shift clicks
    if ((event->button.state & GDK_SHIFT_MASK) == 0)
      ui_interface_mouse_set(iface, MOUSE_1BUTTON_MOTION);

    mouse_shift_click(tv, &temp);

    cairo_region_t *crr;
    crr = cairo_region_create_rectangle(
             (cairo_rectangle_int_t *)&iface->mete_box);
    gdk_window_invalidate_region(event->button.window, crr, FALSE);
    cairo_region_destroy(crr);
  }
  return TRUE;
}

static _Bool
mouse_press_event_btn(LCIFindPort *fport, GdkEvent *event, PhxObject *obj) {

  if (obj == fport->object_list[choose_box]) {
    findport_combo_run(fport, (PhxObjectButton*)obj);
    return TRUE;
  }
  if (obj == fport->object_list[navigate_left_box]) {
    findport_navigate_left(fport);
    return TRUE;
  }
  if (obj == fport->object_list[navigate_right_box]) {
    findport_navigate_right(fport);
    return TRUE;
  }
  if (obj == fport->object_list[close0_box]) {
      // do nothing until release, button drawing issue
    return TRUE;
  }
  if (obj == fport->object_list[replace_all_box]) {
    findport_replace_all(fport);
    return TRUE;
  }
  if (obj == fport->object_list[replace_box]) {
    findport_replace(fport);
    return TRUE;
  }
  if (obj == fport->object_list[replace_find_box]) {
    findport_replace_and_find(fport);
    return TRUE;
  }
  if (obj == fport->object_list[close1_box]) {
      // do nothing until release
    return TRUE;
  }
  if ((obj == fport->object_list[textview_replace_box])
      || (obj == fport->object_list[textview_find_box])) {
    return mouse_press_event_txt((PhxInterface*)fport, event, obj);
  }
  return FALSE;
}

static _Bool
mouse_release_event_btn(LCIFindPort *fport, GdkEvent *event, PhxObject *obj) {

  if ( (obj == fport->object_list[close0_box])
      || (obj == fport->object_list[close1_box]) ) {
    lci_findport_clear_results(fport);
    gtk_widget_hide(main_window);
    visible_set((PhxObject*)fport, FALSE);
  }
  return TRUE;
}

// when vertical scrolling, at least 1 line must display at end
// base on 1/10 of bin height, ceiling of line heights
// bin of 300 => 20l@15p would do (300/10 + 14) / 15 => 2 lines
// bin of 750 => 50l              (75+14)/15 => 5
static _Bool
mouse_scroll_event(PhxInterface *iface, GdkEvent *event) {

  PhxObject *obj = iface->has_focus;
  if ( (obj == NULL)
      || ((obj->type != PHX_TEXTVIEW) && (obj->type != PHX_ENTRY)) ) {
    return TRUE;
  }
  PhxObjectTextview *otxt = (PhxObjectTextview*)obj;

  int y, font_em = otxt->font_em;

  y = (otxt->bin.h - otxt->bin.y) / 10;
  if (y <= font_em)
    y = font_em;
  else
    y = ((y + (font_em - 1)) / font_em) * font_em;

  if (event->scroll.direction == GDK_SCROLL_UP)
    y = ((otxt->bin.y / font_em) * font_em) - y;
  else
    y += ((otxt->bin.h - font_em) / font_em) * font_em;

  location scroll = { otxt->bin.x, y, 0 };
  location_for_point(otxt, &scroll);
  location_auto_scroll(otxt, &scroll);

  cairo_region_t *crr;
  crr = cairo_region_create_rectangle(
           (cairo_rectangle_int_t *)&iface->mete_box);
  gdk_window_invalidate_region(event->scroll.window, crr, FALSE);
  cairo_region_destroy(crr);
  return TRUE;
}

static int
key_movement_page_scroll(PhxObjectTextview *tv, short dirdis) {

  int font_em = tv->font_em;
  int scroll_y = tv->bin.y;
  if (dirdis < 0) {  // GDK_KEY_Page_Up
    if (tv->bin.y != 0) {
        // check for case of less than two line viewing
      if (tv->draw_box.h < (font_em << 1)) {
        tv->bin.y -= font_em;
      } else {
          // move partial exposed below view
        int delta = tv->bin.y - ((tv->bin.y / font_em) * font_em);
        if (delta != 0)  tv->bin.y -= delta - font_em;
          // move full viewed top line to bottom of viewed page
        tv->bin.y -= tv->draw_box.h - font_em;
        tv->bin.h = tv->bin.y + tv->draw_box.h;

        delta = tv->bin.h - ((tv->bin.h / font_em) * font_em);
        if (delta != 0) {
printf("key_movement_page_scroll %d\n", delta);
          tv->bin.y += font_em;
        }
      }
      if (tv->bin.y < 0)  tv->bin.y = 0;
    }
  } else {           // GDK_KEY_Page_Down
    location temp;
    temp.offset = (int)strlen(tv->string);
    location_for_offset(tv, &temp);
    if ((tv->bin.y + tv->draw_box.h - font_em) <= temp.y) {
      if (tv->draw_box.h < (font_em << 1))
            tv->bin.y += font_em;
      else  tv->bin.y = ((tv->bin.h / font_em) * font_em) - font_em;
    }
  }
  tv->bin.h = tv->bin.y + tv->draw_box.h;
  return (tv->bin.y - scroll_y);
}

static int
key_movement_control(PhxObjectTextview *tv, location *active,
                                     location *inactive, short event_flag) {
  int font_em = tv->font_em;
  int offset, shift_click = event_flag & GDK_SHIFT_MASK;
  short dirdis = event_flag >> 4;
  char ch0, *cPtr = &tv->string[active->offset];
  ch0 = *cPtr;
    // horizontal movement to begin/end of previous/next identifiers
  if ((dirdis & 3) == 0) {
    if (dirdis < 0) {  // GDK_KEY_Left
      if ((cPtr - 1) < tv->string)  return TRUE;
      ch0 = *(--cPtr);
      if ((isalnum(ch0) == 0) && (ch0 != '_'))
        do {
          if ((cPtr - 1) < tv->string)  goto mcl;
          ch0 = *(cPtr - 1);
          if ((isalnum(ch0) != 0) || (ch0 == '_'))  break;
          --cPtr;
        } while (1);
      do {
        if ((cPtr - 1) < tv->string)  break;
        ch0 = *(cPtr - 1);
        if ((isalnum(ch0) == 0) && (ch0 != '_'))  break;
        --cPtr;
      } while (1);
mcl:  offset = (int)(cPtr - tv->string);
      if ( (shift_click && (offset < active->offset))
          || !shift_click )
        goto mc0;
    } else {           // GDK_KEY_Right
      if (ch0 == 0)  return TRUE;
      if ((isalnum(ch0) == 0) && (ch0 != '_'))
        do {
          if ((ch0 = *(++cPtr)) == 0)  goto mcr;
        } while ((isalnum(ch0) == 0) && (ch0 != '_'));
      do {
        if ((ch0 = *(++cPtr)) == 0)  break;
      } while ((isalnum(ch0) != 0) || (ch0 == '_'));
mcr:  offset = (int)(cPtr - tv->string);
      if ( (shift_click && (offset > active->offset))
          || !shift_click ) {
  mc0:  active->offset = offset;
        location_for_offset(tv, active);
        location_auto_scroll(tv, active);
      }
    }
    goto kmc;
  }

  if ((dirdis & 3) == 2) {
    char *tempPtr;
    if (dirdis < 0) {  // GDK_KEY_Home
      if (active->offset != 0) {
        tempPtr = memrchr(tv->string, '\n', active->offset);
        tempPtr = (tempPtr == NULL) ? tv->string : (tempPtr + 1);
        if ((ch0 = *tempPtr) != 0)
          while (isblank(ch0))  ch0 = *(++tempPtr);
        active->offset = (int)(tempPtr - tv->string);
          // sum of glyph advances
        location_for_offset(tv, active);
          // auto-scrolls 'tv->bin.x' amount (reduced for begin only)
        tv->bin.x = 0;
        tv->bin.w = tv->draw_box.w;
      }
    } else {           // GDK_KEY_End
      tempPtr = strchr(cPtr, '\n');
      if (tempPtr == NULL) tempPtr = cPtr + (int)strlen(cPtr);
      while ((tempPtr - 1) >= cPtr) {
        ch0 = *(tempPtr - 1);
        if (!isblank(ch0))  break;
        tempPtr--;
      }
      active->offset = (int)(tempPtr - tv->string);
        // sum of glyph advances
      location_for_offset(tv, active);
        // x auto-scroll (reduced for end only)
      int x = active->x + tv->bin.x;
      if (x >= (tv->bin.w - font_em)) {
        int delta = x - (tv->bin.w - font_em);
        tv->bin.x += delta;
        tv->bin.w += delta;
      }
    }
    goto kmc;
  }

  if ((dirdis & 3) == 3) {
    if (dirdis < 0) {  // GDK_KEY_Page_Up
      active->x = (active->y = (active->offset = 0));
      tv->bin.x = (tv->bin.y = 0);
      tv->bin.w = tv->draw_box.w;
      tv->bin.h = tv->draw_box.h;
    } else {           // GDK_KEY_Page_Down
      active->offset = (int)strlen(tv->string);
      location_for_offset(tv, active);
      location_auto_scroll(tv, active);
    }
kmc:
    if (shift_click && (inactive->offset != active->offset))  {
      tv->interim.x = active->x;
      tv->interim.y = active->y;
      tv->interim.offset = active->offset;
    } else {
      tv->interim.x = (inactive->x = active->x);
      tv->interim.y = (inactive->y = active->y);
      tv->interim.offset = (inactive->offset = active->offset);
    }
    return TRUE;
  }
  return FALSE;
}

static void
key_movement_press(PhxObjectTextview *tv, short event_flag) {

  location *active, *inactive;

    // flatten text_buffer, if was editing
  text_buffer_reset(tv);

  int font_em = tv->font_em;

  short dirdis = event_flag >> 4;
    // deselection of selection
  if ( ((event_flag & 0x000F) == 0)
      && (tv->insert.offset != tv->release.offset) ) {
    if (dirdis < 0)
          active = &tv->insert, inactive = &tv->release;
    else  active = &tv->release, inactive = &tv->insert;
    tv->interim.x = (inactive->x = active->x);
    tv->interim.y = (inactive->y = active->y);
    tv->interim.offset = (inactive->offset = active->offset);
    location_auto_scroll(tv, active);
    return;
  }

  active = &tv->release, inactive = &tv->insert;
  if (tv->interim.offset == tv->insert.offset) {
    active = &tv->insert, inactive = &tv->release;
    if ( (tv->interim.x == tv->insert.x)
        && (tv->interim.y == tv->insert.y) ) {
NWSE:   if (dirdis < 0)
            active = &tv->insert, inactive = &tv->release;
      else  active = &tv->release, inactive = &tv->insert;
      tv->interim.x = active->x;
      tv->interim.y = active->y;
      tv->interim.offset = active->offset;
    }
  } else if ( (tv->interim.x == tv->release.x)
             && (tv->interim.y == tv->release.y) ) {
    goto NWSE;
  }

  if (event_flag & GDK_CONTROL_MASK) {
    if (key_movement_control(tv, active, inactive, event_flag) == TRUE)
      return;
  }

  if ((dirdis & 3) == 0) {
    if (dirdis < 0) {  // GDK_KEY_Left
      if (active->offset != 0)              active->offset--;
    } else {           // GDK_KEY_Right
      if (tv->string[active->offset] != 0)  active->offset++;
    }
    location_for_offset(tv, active);
    location_auto_scroll(tv, active);
    goto kmp_h;
  }

  if ((dirdis & 3) == 1) {
    if (dirdis < 0) {  // GDK_KEY_Up
      if (active->y == 0)  goto kmp_v;
       active->y -= font_em;
    } else {           // GDK_KEY_Down
      active->y += font_em;
    }
    goto kmp_v;
  }

  if ((dirdis & 3) == 2) {
    char *tempPtr;
    if (dirdis < 0) {  // GDK_KEY_Home
      if (active->offset != 0) {
        tempPtr = memrchr(tv->string, '\n', active->offset);
        tempPtr = (tempPtr == NULL) ? tv->string : (tempPtr + 1);
        active->offset = (int)(tempPtr - tv->string);
          // auto-scrolls 'tv->bin.x' amount (reduced for begin only)
        active->x = 0;
        tv->bin.x = 0;
        tv->bin.w = tv->draw_box.w;
      }
    } else {           // GDK_KEY_End
      tempPtr = strchr(&tv->string[active->offset], '\n');
      active->offset = (int)(tempPtr - tv->string);
      if (tempPtr == NULL) active->offset = (int)strlen(tv->string);
        // sum of glyph advances
      location_for_offset(tv, active);
        // x auto-scroll (reduced for end only)
      int x = active->x + tv->bin.x;
      if (x >= (tv->bin.w - font_em)) {
        int delta = x - (tv->bin.w - font_em);
        tv->bin.x += delta;
        tv->bin.w += delta;
      }
    }
kmp_h:
    if ( ((event_flag & GDK_SHIFT_MASK) != 0)
        && (inactive->offset != active->offset) )  {
      tv->interim.x = active->x;
      tv->interim.y = active->y;
      tv->interim.offset = active->offset;
    } else {
      tv->interim.x = (inactive->x = active->x);
      tv->interim.y = (inactive->y = active->y);
      tv->interim.offset = (inactive->offset = active->offset);
    }
    return;
  }
  if ((dirdis & 3) == 3) {
      // move bin, adjust y by amount moved
    active->y += key_movement_page_scroll(tv, dirdis);
kmp_v:
    active->x = tv->interim.x;
    location_for_point(tv, active);
    location_auto_scroll(tv, active);
    if ( ((event_flag & GDK_SHIFT_MASK) != 0)
        && (inactive->offset != active->offset) )  {
      tv->interim.y = active->y;
      tv->interim.offset = active->offset;
    } else {
      inactive->x = active->x;
      tv->interim.y = (inactive->y = active->y);
      tv->interim.offset = (inactive->offset = active->offset);
    }
    return;
  }
}

static gboolean
key_press_event(PhxInterface *iface, GdkEvent *event, PhxObject *obj) {

  short state = event->key.state
                 & (GDK_MOD1_MASK | GDK_CONTROL_MASK | GDK_SHIFT_MASK);
  int ch = event->key.keyval;

  if ( (obj->type != PHX_TEXTVIEW) && (obj->type != PHX_ENTRY) ) {
    if ( (iface == (PhxInterface*)findPort)
        && (ch == 'f') && (state & GDK_CONTROL_MASK) ) {
      lci_findport_search(findPort);
      return TRUE;
    }
puts("obj not of 'focus' type");
    return FALSE;
  }

  PhxObjectTextview *tv = (PhxObjectTextview*)obj;

  if ( (ch >= GDK_KEY_space) && (ch <= GDK_KEY_asciitilde) ) {
    if (state & GDK_CONTROL_MASK) {
      if (ch == 'a') {  text_buffer_select_all(tv);  return TRUE;  }
      if (ch == 'c') {  text_buffer_copy(tv);  return TRUE;  }
      if ( (ch == 'e') || (ch == 'E') ) {
        if (findPort != NULL) {
          size_t sz = tv->release.offset - tv->insert.offset;
          if (sz != 0) {
            char *data = &tv->string[tv->insert.offset];
            lci_findport_receiver_text(findPort, ch, data, sz);
          }
        }
        return TRUE;
      }
      if (ch == 'f') {
          // find bar needs and expects iface->param
          // to be set so that it can search a textview
          // with its 'search entry' data
        if ( (main_window != NULL) && (!gtk_widget_is_visible(main_window)) ) {
          gtk_widget_show_all(main_window);
          visible_set((PhxObject*)findPort, TRUE);
        }
        lci_findport_search(findPort);
        return TRUE;
      }
      if (ch == 'v') {  text_buffer_paste(tv); goto redraw;  }
      if (ch == 'x') {  text_buffer_cut(tv);   goto redraw;  }
      return FALSE;
    }
    text_buffer_insert(tv, (char*)&ch, 1);
    goto redraw;
  }

  switch (event->key.keyval) {

    case GDK_KEY_BackSpace:                   // 0xff08
      text_buffer_delete(tv);
      goto redraw;

    case GDK_KEY_Tab:                         // 0xff09
      text_buffer_insert(tv, (char*)&ch, 1);
      goto redraw;

    case GDK_KEY_Return:                      // 0xff0d
    case GDK_KEY_KP_Enter:                    // 0xff8d
      ch = '\n';
      text_buffer_insert(tv, (char*)&ch, 1);
      goto redraw;

    case GDK_KEY_Home:                        // 0xff50
    case GDK_KEY_KP_Home:                     // 0xff95
      key_movement_press(tv, (state | (short)0x8020));
      goto redraw;

    case GDK_KEY_Left:                        // 0xff51
    case GDK_KEY_KP_Left:                     // 0xff96
      key_movement_press(tv, (state | (short)0x8000));
      goto redraw;

    case GDK_KEY_Up:                          // 0xff52
    case GDK_KEY_KP_Up:                       // 0xff97
      key_movement_press(tv, (state | (short)0x8010));
      goto redraw;

    case GDK_KEY_Right:                       // 0xff53
    case GDK_KEY_KP_Right:                    // 0xff98
      key_movement_press(tv, (state | (short)0x0000));
      goto redraw;

    case GDK_KEY_Down:                        // 0xff54
    case GDK_KEY_KP_Down:                     // 0xff99
      key_movement_press(tv, (state | (short)0x0010));
      goto redraw;

    case GDK_KEY_Page_Up:                     // 0xff55
    case GDK_KEY_KP_Page_Up:                  // 0xff9a
    //case GDK_KEY_KP_Prior:                  // 0xff9a
      key_movement_press(tv, (state | (short)0x8030));
      goto redraw;

    case GDK_KEY_Page_Down:                   // 0xff56
    case GDK_KEY_KP_Page_Down:                // 0xff9b
    //case GDK_KEY_KP_Next:                   // 0xff9b
      key_movement_press(tv, (state | (short)0x0030));
      goto redraw;

    case GDK_KEY_End:                         // 0xff57
    case GDK_KEY_KP_End:                      // 0xff9c
      key_movement_press(tv, (state | (short)0x0020));
      goto redraw;

    case GDK_KEY_KP_Begin:                    // 0xff9d
      return FALSE;

    case GDK_KEY_Insert:                      // 0xff63
    case GDK_KEY_KP_Insert:                   // 0xff9e
      tv->state ^= (0x00000001 << (SHIFT + 10));
      goto redraw;

    case GDK_KEY_Menu:                        // 0xff67
    case GDK_KEY_Num_Lock:                    // 0xff7f
      return FALSE;

    case GDK_KEY_KP_Multiply:                 // 0xffaa * 2a
    case GDK_KEY_KP_Add:                      // 0xffab + 2b
    case GDK_KEY_KP_Separator:                // 0xffac , 2c
    case GDK_KEY_KP_Subtract:                 // 0xffad - 2d
    case GDK_KEY_KP_Decimal:                  // 0xffae . 2e
    case GDK_KEY_KP_Divide:                   // 0xffaf / 2f
    case GDK_KEY_KP_0:                        // 0xffb0
    case GDK_KEY_KP_1:                        // 0xffb1
    case GDK_KEY_KP_2:                        // 0xffb2
    case GDK_KEY_KP_3:                        // 0xffb3
    case GDK_KEY_KP_4:                        // 0xffb4
    case GDK_KEY_KP_5:                        // 0xffb5
    case GDK_KEY_KP_6:                        // 0xffb6
    case GDK_KEY_KP_7:                        // 0xffb7
    case GDK_KEY_KP_8:                        // 0xffb8
    case GDK_KEY_KP_9:                        // 0xffb9
      ch &= 0x3F;
      text_buffer_insert(tv, (char*)&ch, 1);
      goto redraw;

    case GDK_KEY_Shift_L:                     // 0xffe1
    case GDK_KEY_Shift_R:                     // 0xffe2
    case GDK_KEY_Control_L:                   // 0xffe3
    case GDK_KEY_Control_R:                   // 0xffe4
    case GDK_KEY_Caps_Lock:                   // 0xffe5
    case GDK_KEY_Shift_Lock:                  // 0xffe6
    case GDK_KEY_Alt_L:                       // 0xffe9
    case GDK_KEY_Alt_R:                       // 0xffea
    case GDK_KEY_Super_L:                     // 0xffeb
      return FALSE;

    case GDK_KEY_Delete:                      // 0xffff
    case GDK_KEY_KP_Delete:                   // 0xff9f
      tv->state ^= (0x00000001 << (SHIFT + 11));
      text_buffer_delete(tv);
      goto redraw;

    default:
printf("unhandled key (0x%X)\n", event->key.keyval);
  }
  return FALSE;

  cairo_region_t *crr;
redraw:
  crr = cairo_region_create_rectangle(
           (cairo_rectangle_int_t *)&iface->mete_box);
  gdk_window_invalidate_region(iface->parent_window, crr, FALSE);
  cairo_region_destroy(crr);
  return TRUE;
}

/*
  GDK_MOTION_NOTIFY  = 3,
  GDK_BUTTON_PRESS   = 4,
  GDK_2BUTTON_PRESS  = 5,
  GDK_DOUBLE_BUTTON_PRESS = GDK_2BUTTON_PRESS,
  GDK_3BUTTON_PRESS  = 6,
  GDK_TRIPLE_BUTTON_PRESS = GDK_3BUTTON_PRESS,
  GDK_BUTTON_RELEASE = 7,
  GDK_KEY_PRESS      = 8,
  GDK_KEY_RELEASE    = 9,
  GDK_ENTER_NOTIFY   = 10,
  GDK_LEAVE_NOTIFY   = 11,
  GDK_FOCUS_CHANGE   = 12,
// don't recieve this, must be top-most window
  GDK_CONFIGURE      = 13,
  GDK_SCROLL         = 31,
*/
static int
event_meter(PhxInterface *iface, GdkEvent *event, void *widget) {

  (void)widget;

  unsigned type = (unsigned)(event->type - GDK_MOTION_NOTIFY);
  if (type > (unsigned)(GDK_FOCUS_CHANGE - GDK_MOTION_NOTIFY)) {

    if (event->type == GDK_SCROLL)
      return mouse_scroll_event(iface, event);
    return FALSE;
  }
  if ( (event->type >= GDK_MOTION_NOTIFY)
       && (event->type <= GDK_BUTTON_RELEASE) ) {

    unsigned button;
    if (gdk_event_get_button(event, &button)) {
      if (button != 1)  return TRUE;
    }

    if ((ui_interface_mouse_get(iface) & MOUSE_DND) == MOUSE_DND) {
      if (event->type == GDK_MOTION_NOTIFY)
        return mouse_drag_motion(iface, event);
      if (event->type == GDK_BUTTON_RELEASE)
        return mouse_drag_finish(iface, event);
    }

    int x = (int)(event->button.x);
    int y = (int)(event->button.y);
    int ldx = 0;
    if ( ((unsigned)x <= (unsigned)iface->mete_box.w)
        && ((unsigned)y <= (unsigned)iface->mete_box.h))
      ldx = iface->event_map[x][y];
    PhxObject *obj = iface->object_list[ldx];

    if ((event->type >= GDK_BUTTON_PRESS)
        && (event->type < GDK_BUTTON_RELEASE)) {

      if (obj->type == PHX_LABEL)  return TRUE;
      if (ldx == 0)  return TRUE;
      if (!visible_get(obj))  return TRUE;
      if ((obj->type != PHX_TEXTVIEW) && (obj->type != PHX_ENTRY)) {
          // obj's 'activate on click'
        if (!sensitive_get(obj))  return TRUE;
        obj->state |= 1;
        iface->has_focus = obj;
          // queue_redraw
        cairo_region_t *crr;
        crr = cairo_region_create_rectangle(
                 (cairo_rectangle_int_t *)&obj->draw_box);
        gdk_window_invalidate_region(event->button.window, crr, FALSE);
        cairo_region_destroy(crr);
          // perform action
        return mouse_press_event_btn((LCIFindPort*)iface, event, obj);
      }
        // perform action
      return mouse_press_event_txt(iface, event, obj);

    } else if (event->type == GDK_BUTTON_RELEASE) {

      PhxObject *new_obj = (iface->has_focus == NULL) ? obj : iface->has_focus;
      if ((new_obj->type != PHX_TEXTVIEW) && (new_obj->type != PHX_ENTRY)) {
          // obj's 'activate on click release'
          // text_reset can alter sensitive
//        if (!sensitive_get(new_obj))  return TRUE;
        new_obj->state &= ~7;
          // queue_redraw
        cairo_region_t *crr;
        crr = cairo_region_create_rectangle(
                 (cairo_rectangle_int_t *)&new_obj->draw_box);
        gdk_window_invalidate_region(event->button.window, crr, FALSE);
        cairo_region_destroy(crr);
        return mouse_release_event_btn((LCIFindPort*)iface, event, obj);
      }
      return mouse_release_event_txt(iface, event, obj);

    } else if (event->type == GDK_MOTION_NOTIFY) {

      PhxObject *new_obj = (iface->has_focus == NULL) ? obj : iface->has_focus;
      if ((new_obj->type == PHX_TEXTVIEW) || (new_obj->type == PHX_ENTRY)) {
        GdkModifierType state;
        if (gdk_event_get_state(event, &state)
                  && (state & GDK_BUTTON1_MASK)) {
          return mouse_motion_event_txt(iface, event, new_obj);
        }
      }
      char *named = NULL;
      if ((obj->type == PHX_TEXTVIEW) || (obj->type == PHX_ENTRY))
        named = "text";
      ui_interface_set_cursor_named(iface->parent_window, named);
    }
    return TRUE;

  } else {

    if (event->type == GDK_FOCUS_CHANGE) {
      PhxObject *fobj = iface->has_focus;
      if ( (fobj != NULL)
          && ((fobj->type == PHX_TEXTVIEW) || (fobj->type == PHX_ENTRY)) )
        sensitive_set(fobj, event->focus_change.in);
          //queue_redraw
      cairo_region_t *crr;
      crr = cairo_region_create_rectangle(
               (cairo_rectangle_int_t *)&iface->mete_box);
      gdk_window_invalidate_region(event->button.window, crr, FALSE);
      cairo_region_destroy(crr);
      return TRUE;
    }
    if (event->type == GDK_ENTER_NOTIFY) {
      GdkWindow *window = event->crossing.window;
      GdkDisplay *display = gdk_window_get_display(window);
      GdkCursor *cursor = gdk_cursor_new_from_name(display, "text");
      gdk_window_set_cursor(window, cursor);
      return TRUE;
    }
    if (event->type == GDK_LEAVE_NOTIFY) {
      if (event->crossing.window == iface->parent_window)
        return TRUE;

      GdkWindow *window = event->crossing.window;
      GdkCursor *cursor = gdk_window_get_cursor(window);
      if (cursor != NULL) {
        gdk_window_set_cursor(window, NULL);
        g_object_unref((void*)cursor);
      }
      return TRUE;
    }
    if (event->type == GDK_KEY_PRESS) {

      BState cstate = ui_interface_mouse_get(iface);
      if ((cstate & MOUSE_DND) == MOUSE_DND) {
        if ( (event->key.keyval == GDK_KEY_Control_L)
            || (event->key.keyval == GDK_KEY_Control_R) )
          if (cstate != DND_COPY_CURSOR) {
            // need to verify is textview
            ui_interface_mouse_set(iface, DND_COPY_CURSOR);
            ui_interface_set_cursor_named(iface->parent_window, "copy");
            return TRUE;
          }
      }
      if (iface->has_focus == NULL)  return FALSE;
      return key_press_event(iface, event, iface->has_focus);
    }
    if (event->type == GDK_KEY_RELEASE) {

      BState cstate = ui_interface_mouse_get(iface);
      if (cstate == DND_COPY_CURSOR) {
        if ( (event->key.keyval == GDK_KEY_Control_L)
            || (event->key.keyval == GDK_KEY_Control_R) ) {
            // need to verify is textview
          ui_interface_mouse_set(iface, DND_MOVE_CURSOR);
          ui_interface_set_cursor_named(iface->parent_window, "move");
          return TRUE;
        }
      }
      return FALSE;
    }
  }
  return FALSE;
}

#pragma mark *** Main ***

static void
fw_initialize(PhxInterface *fport, cairo_t *cro) {

  PhxObjectButton   *obtn;
  PhxObjectTextview *otxt;
  PhxObjectLabel    *olbl;

  int xpos = 0;
  int box_height = BOX_HEIGHT;

  int mdx = (box_height < 21) ? 0 : 1;
  int bbm = window_adjustments[mdx][2];

                        /* Combo Button [0,0] */
  obtn = (PhxObjectButton*)ui_object_create(PHX_BUTTON_COMBO, draw_button,
                         xpos, 0, fport->mete_box.w, box_height);
  ui_box_inset(&obtn->draw_box, bbm, bbm, bbm, bbm);
  ui_button_menu_create(obtn, cro, 2, "Find", "Find & Replace");
  ui_interface_add(fport, (PhxObject*)obtn);

// want decrease in between button drawing, vertical alter mete location
// problem later if create mete_box clip
  int alter_y = bbm + 1;
  int alter_x = bbm;

                        /* Simple Button [1,0] */
    // want this button size same as the combo button's, consider as max,min
  int combo_width = obtn->mete_box.w;
  obtn = (PhxObjectButton*)ui_object_create(PHX_BUTTON_LABELED, draw_button,
                        xpos, (box_height - alter_y), combo_width, box_height);
  ui_box_inset(&obtn->draw_box, bbm, bbm, bbm, bbm);
  int whtsp = ui_button_label_create(obtn, cro, "Replace All", HJST_CTR);
  ui_interface_add(fport, (PhxObject*)obtn);
  visible_set((PhxObject*)obtn, FALSE);

                        /* Simple Button [1,1] */
  xpos += obtn->mete_box.w - alter_x;
    // using 'combo_width' as starting size, the max
  obtn = (PhxObjectButton*)ui_object_create(PHX_BUTTON_LABELED, draw_button,
                        xpos, (box_height - alter_y), combo_width, box_height);
  ui_box_inset(&obtn->draw_box, bbm, bbm, bbm, bbm);
    // want adjustments to width, used prior width to get difference
  whtsp = (ui_button_label_create(obtn, cro, "Replace", HJST_CTR) - whtsp) * 2;
  ui_box_inset(&obtn->label->draw_box, 0, 0, whtsp, 0);
  ui_box_inset(&obtn->label->bin,      0, 0, whtsp, 0);
  ui_box_inset(&obtn->draw_box,        0, 0, whtsp, 0);
  ui_box_inset(&obtn->mete_box,        0, 0, whtsp, 0);
  ui_interface_add(fport, (PhxObject*)obtn);
  visible_set((PhxObject*)obtn, FALSE);

                        /* Simple Button [1,2] */
  xpos += obtn->mete_box.w - alter_x;
    // this button same size as column 0
  obtn = (PhxObjectButton*)ui_object_create(PHX_BUTTON_LABELED, draw_button,
                        xpos, (box_height - alter_y), combo_width, box_height);
  ui_box_inset(&obtn->draw_box, bbm, bbm, bbm, bbm);
  ui_button_label_create(obtn, cro, "Replace & Find", HJST_CTR);
  ui_interface_add(fport, (PhxObject*)obtn);
  visible_set((PhxObject*)obtn, FALSE);

                        /* Simple Button [1,4] */
    // last in row, right justified
    // xpos: keep for textview start
  xpos += obtn->mete_box.w;
    // this starts at fport width ends at -close width
  obtn = (PhxObjectButton*)ui_object_create(PHX_BUTTON_LABELED, draw_button,
                     (fport->mete_box.w - combo_width), (box_height - alter_y),
                     combo_width, box_height);
  ui_box_inset(&obtn->draw_box, bbm, bbm, bbm, bbm);
  whtsp = ui_button_label_create(obtn, cro, "Done", HJST_CTR);
    // desired size change => draw_box.w - text_width
  whtsp = obtn->label->draw_box.w - (whtsp + 6);
  ui_box_inset(&obtn->label->draw_box, whtsp, 0, -whtsp, 0);
  ui_box_inset(&obtn->draw_box, (2 * whtsp), 0, 0, 0);
  ui_box_inset(&obtn->mete_box, (2 * whtsp), 0, 0, 0);
  ui_interface_add(fport, (PhxObject*)obtn);
  visible_set((PhxObject*)obtn, FALSE);

                        /* Simple Button [0,4] */
  int done_width = obtn->mete_box.w;
  obtn = (PhxObjectButton*)ui_object_create(PHX_BUTTON_LABELED, draw_button,
                  (fport->mete_box.w - done_width), 0, done_width, box_height);
  ui_box_inset(&obtn->draw_box, bbm, bbm, bbm, bbm);
  ui_button_label_create(obtn, cro, "Done", HJST_CTR);
  ui_interface_add(fport, (PhxObject*)obtn);

                        /* Textview [1,3] */
  otxt = (PhxObjectTextview*)ui_object_create(PHX_ENTRY, draw_textview,
                         xpos, box_height - 1,
                         obtn->mete_box.x - xpos, (box_height - (2 * bbm)));
  ui_box_inset(&otxt->draw_box, 1, 1, 1, 1);
  ui_textview_font_set(otxt, cro, otxt->draw_box.h);
  ui_textview_buffer_set(otxt, "replacing_text");
  ui_interface_add(fport, (PhxObject*)otxt);
  visible_set((PhxObject*)otxt, FALSE);

                        /* Textview [0,3] */
  otxt = (PhxObjectTextview*)ui_object_create(PHX_ENTRY, draw_textview,
                         xpos, bbm,
                         obtn->mete_box.x - xpos, (box_height - (2 * bbm)));
  ui_box_inset(&otxt->draw_box, 1, 1, 1, 1);
  ui_textview_font_set(otxt, cro, otxt->draw_box.h);
  ui_textview_buffer_set(otxt, "searched_text");
  ui_interface_add(fport, (PhxObject*)otxt);

                        /* Navigation Button [0,2] */
                        /* Navigation Button [0,2.5] */
    // should be 2 square buttons joined, child is drawing instead of label
  xpos = otxt->mete_box.x;
  obtn = (PhxObjectButton*)ui_object_create(PHX_NAVIGATE_RIGHT, draw_navigate,
                                             (xpos - (box_height - bbm)), 0,
                                             (box_height - bbm), box_height);
  ui_box_inset(&obtn->draw_box, 0, bbm, bbm, bbm);
  ui_interface_add(fport, (PhxObject*)obtn);
  sensitive_set((PhxObject*)obtn, FALSE);
  xpos = obtn->mete_box.x;
  obtn = (PhxObjectButton*)ui_object_create(PHX_NAVIGATE_LEFT, draw_navigate,
                                             (xpos - (box_height - bbm)), 0,
                                             (box_height - bbm), box_height);
  ui_box_inset(&obtn->draw_box, bbm, bbm, 0, bbm);
  sensitive_set((PhxObject*)obtn, FALSE);
  ui_interface_add(fport, (PhxObject*)obtn);

                        /* Label [0,1] */
  xpos = obtn->mete_box.x;
  int cbw = fport->object_list[choose_box]->mete_box.w;
  olbl = (PhxObjectLabel*)ui_object_create(PHX_LABEL, draw_label,
                                           cbw, 0, (xpos - cbw), box_height);
  ui_box_inset(&olbl->draw_box, bbm, box_height/3.5, bbm, bbm);
  ui_label_set(olbl, cro, "0 found matches", HJST_RGT);
  ui_interface_add(fport, (PhxObject*)olbl);
}

static void
tw_initialize(PhxInterface *tport, cairo_t *cro) {

  PhxObjectTextview *otxt;

// single object create
  char *buffer;
  long filesize;
  FILE *rh
//  = fopen("/home/steven/Development/Projects/find_bar/internal buffer", "r");
      = fopen("/home/steven/Development/Projects/find_bar/find_bar.c", "r");
  if (rh == NULL) {  puts("file not found"); return;  }
  fseek(rh, 0 , SEEK_END);
  filesize = ftell(rh);
  fseek(rh, 0 , SEEK_SET);
  size_t rdSz = (filesize + TGAP_ALLOC) & ~(TGAP_ALLOC - 1);
  buffer = malloc(rdSz);
  memset(&buffer[filesize], 0, (rdSz - filesize));
  fread(buffer, filesize, 1, rh);
  fclose(rh);

  otxt = (PhxObjectTextview*)ui_object_create(PHX_TEXTVIEW,
                                draw_textview, 0, 0, TW, TH);
    // Special Note: setting margins here, instead of after
    // ui_interface_add(), will omit margin areas from events.
  ui_box_inset(&otxt->draw_box, 1, 1, 1, 1);
  ui_textview_font_set(otxt, cro, 16);
  ui_textview_buffer_set(otxt, buffer);
  ui_interface_add(tport, (PhxObject*)otxt);

  free(buffer);
}

static _Bool
close_window(PhxInterface *iface, GdkEvent *event, GtkWidget *widget) {
  lci_findport_clear_results(findPort);
  gtk_widget_hide(widget);
  visible_set((PhxObject*)iface, FALSE);
  return TRUE;
}

int
main(int argc, char *argv[]) {

  cairo_surface_t   *surface;
  cairo_t           *cro;
  PhxInterface      *tport;
  PhxObjectDrawing  *odrw;

  gtk_init(&argc, &argv); // initialize Gtk

  GdkDisplay *display = gdk_display_get_default();
  clipboard
        = gtk_clipboard_get_for_display(display, GDK_SELECTION_CLIPBOARD);

/* Need textview of file for testing */
  GtkWidget *text_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size(GTK_WINDOW(text_window), TW, TH);

  GtkWidget *textview = gtk_drawing_area_new();
  gtk_container_add(GTK_CONTAINER(text_window), textview);

  gtk_widget_realize(text_window);
  gtk_widget_realize(textview);

  tport = ui_interface_create((GtkDrawingArea*)textview, 0, 0, TW, TH);

    // signals for G_OBJECT(text_window)
  g_signal_connect(G_OBJECT(text_window), "destroy",
                                   G_CALLBACK(gtk_main_quit), NULL);
  gtk_widget_add_events(text_window, GDK_STRUCTURE_MASK
                                | GDK_ENTER_NOTIFY_MASK
                                | GDK_LEAVE_NOTIFY_MASK
                                | GDK_FOCUS_CHANGE_MASK);
  g_signal_connect_swapped(G_OBJECT(textview), "configure-event",
                                G_CALLBACK(configure_event_txtwnd), tport);
    // NOTE: can't change cursor unless connect to top-most
  g_signal_connect_swapped(G_OBJECT(text_window), "enter-notify-event",
                                G_CALLBACK(event_meter), tport);
  g_signal_connect_swapped(G_OBJECT(text_window), "leave-notify-event",
                                G_CALLBACK(event_meter), tport);
  g_signal_connect_swapped(G_OBJECT(text_window), "focus-in-event",
                                G_CALLBACK(event_meter), tport);
  g_signal_connect_swapped(G_OBJECT(text_window), "focus-out-event",
                                G_CALLBACK(event_meter), tport);

    // signals for G_OBJECT(textview)
  g_signal_connect_swapped(G_OBJECT(textview), "draw",
                                G_CALLBACK(text_draw_event), tport);
  gtk_widget_add_events(textview, GDK_BUTTON_PRESS_MASK
                                | GDK_BUTTON_RELEASE_MASK
                                | GDK_BUTTON1_MOTION_MASK
                                | GDK_KEY_PRESS_MASK
                                | GDK_KEY_RELEASE_MASK
                                | GDK_LEAVE_NOTIFY_MASK
                                | GDK_SCROLL_MASK);
  g_signal_connect_swapped(G_OBJECT(textview), "motion-notify-event",
                                G_CALLBACK(event_meter), tport);
  g_signal_connect_swapped(G_OBJECT(textview), "button-press-event",
                                G_CALLBACK(event_meter), tport);
  g_signal_connect_swapped(G_OBJECT(textview), "button-release-event",
                                G_CALLBACK(event_meter), tport);
  gtk_widget_set_can_focus(textview, TRUE);
  g_signal_connect_swapped(G_OBJECT(textview), "key-press-event",
                                G_CALLBACK(event_meter), tport);
  g_signal_connect_swapped(G_OBJECT(textview), "key-release-event",
                                G_CALLBACK(event_meter), tport);
  g_signal_connect_swapped(G_OBJECT(textview), "leave-notify-event",
                                G_CALLBACK(event_meter), tport);
  g_signal_connect_swapped(G_OBJECT(textview), "scroll-event",
                                G_CALLBACK(event_meter), tport);

  surface = gdk_window_create_similar_surface(
                                    tport->parent_window,
                                    CAIRO_CONTENT_COLOR_ALPHA, TW, TH);
  cro = cairo_create(surface);
  cairo_select_font_face(cro, FONT_NAME, CAIRO_FONT_SLANT_NORMAL,
                                         CAIRO_FONT_WEIGHT_NORMAL);
    tw_initialize(tport, cro);
  cairo_destroy(cro);
  cairo_surface_destroy(surface);

  gtk_widget_show_all(text_window);

/* now interface. This will set the searchboard buffer pointer. */
  int window_width = BOX_WIDTH;

  if (BOX_HEIGHT < 14) {
    puts("Can not honor height request < 14.");
    return 0;
  }

  int idx = (BOX_HEIGHT < 21) ? 0 : 1;
  int window_height = (BOX_HEIGHT * 2);
    // adjust view area to for margin differences

  int min_width = (int)((double)BOX_HEIGHT / .0406);
  if (window_width < min_width)
    puts("Forced to run as a dialog window due to requested width.");
    // main viewport
  int two_row_height = window_height + window_adjustments[idx][1];
  main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size(GTK_WINDOW(main_window),
                                            window_width, two_row_height);
  gtk_window_set_resizable(GTK_WINDOW(main_window), TRUE);

  GtkWidget *find_window = gtk_drawing_area_new();
  gtk_widget_set_size_request(find_window, min_width, BOX_HEIGHT);
  gtk_container_add(GTK_CONTAINER(main_window), find_window);

    // get attched to gdk
  gtk_widget_realize(main_window);
  gtk_widget_realize(find_window);

  findPort = (LCIFindPort*)ui_interface_create((GtkDrawingArea*)find_window,
                                        0, 0, window_width, two_row_height);
    // demo has only 1 text file attached
  findPort->param_data = tport->object_list[1];
  visible_set((PhxObject*)findPort, FALSE);

    // signals for G_OBJECT(main_window)
  g_signal_connect_swapped(G_OBJECT(main_window), "delete-event",
                                G_CALLBACK(close_window), findPort);
  gtk_widget_add_events(main_window, GDK_STRUCTURE_MASK
                                   | GDK_FOCUS_CHANGE_MASK);
  g_signal_connect_swapped(G_OBJECT(find_window), "configure-event",
                                G_CALLBACK(configure_event_wnd), findPort);
  g_signal_connect_swapped(G_OBJECT(main_window), "focus-in-event",
                                G_CALLBACK(event_meter), findPort);
  g_signal_connect_swapped(G_OBJECT(main_window), "focus-out-event",
                                G_CALLBACK(event_meter), findPort);

    // signals for G_OBJECT(find_window)
  g_signal_connect_swapped(G_OBJECT(find_window), "draw",
                                G_CALLBACK(find_draw_event), findPort);
    /* Because 'window' will include a textview, need to attach
     * GDK_POINTER_MOTION_MASK to 'window', adjustment to/from
     * pointer/text_cursor */
    // must explicitly connect these
  gtk_widget_add_events(find_window,  GDK_POINTER_MOTION_MASK
                                    | GDK_BUTTON_PRESS_MASK
                                    | GDK_BUTTON_RELEASE_MASK
                                    | GDK_BUTTON1_MOTION_MASK
                                    | GDK_KEY_PRESS_MASK
                                    | GDK_KEY_RELEASE_MASK
                                    | GDK_ENTER_NOTIFY_MASK
                                    | GDK_LEAVE_NOTIFY_MASK
                                    | GDK_SCROLL_MASK);
  g_signal_connect_swapped(G_OBJECT(find_window), "motion-notify-event",
                                G_CALLBACK(event_meter), findPort);
  g_signal_connect_swapped(G_OBJECT(find_window), "button-press-event",
                                G_CALLBACK(event_meter), findPort);
  g_signal_connect_swapped(G_OBJECT(find_window), "button-release-event",
                                G_CALLBACK(event_meter), findPort);
  gtk_widget_set_can_focus(find_window, TRUE);
  g_signal_connect_swapped(G_OBJECT(find_window), "key-press-event",
                                G_CALLBACK(event_meter), findPort);
  g_signal_connect_swapped(G_OBJECT(find_window), "key-release-event",
                                G_CALLBACK(event_meter), tport);
  g_signal_connect_swapped(G_OBJECT(find_window), "leave-notify-event",
                                G_CALLBACK(event_meter), tport);
  g_signal_connect_swapped(G_OBJECT(find_window), "scroll-event",
                                G_CALLBACK(event_meter), tport);

  surface = gdk_window_create_similar_surface(
                                        findPort->parent_window,
                                        CAIRO_CONTENT_COLOR_ALPHA,
                                        window_width, two_row_height);
  cro = cairo_create(surface);
  cairo_select_font_face(cro, FONT_NAME, CAIRO_FONT_SLANT_NORMAL,
                                         CAIRO_FONT_WEIGHT_NORMAL);
    fw_initialize((PhxInterface*)findPort, cro);
  cairo_destroy(cro);
  cairo_surface_destroy(surface);

    // after full drawing of find window, only 'Find' is set visiable
    // resize to match
  gtk_window_resize(GTK_WINDOW(main_window), window_width, BOX_HEIGHT);

  gtk_main();

  free(findPort);
  free(tport);
  return EXIT_SUCCESS;
}
