# find_bar
First ever find bar interface for Linux. This is not a search entry.


  This creates demo/design program showing off first ever find bar for
text editors. This also includes how to design your own buttons, labels,
and the creation of an actual text editor. The text editor is far from
complete. The DnD is internal only.
  I've included lci_findport files. This shows the connections to make
use of a find bar using Gtk. It creates a find bar intended to
mount atop a textview area. It provides find and replace abilities for
a text buffer.

  3 Nov 23. Altered code to fix popup. Went from TOP_MOST to POPUP had
altered event behavior, which found workaround. Could not figure workarounds
on gtk tags multiline display, nor scroll abilities to a selection.

  13 Nov 23. Changes made to expand base code. Initial design now approaching
an almost seperated library of PhxInterfaces and PhxObjects. A few added
features included in design based on not yet released second interface. The
POPUP use is nearing use as treeviews. Still working on 'marks/tags'.

  22 Nov 23. Major shift in code. Seperated PhxObjects for a soon to be a static
library for link in. Marks had some work done on them, and more complete use of
'pop-up'. Addressed issue of an OS using a different coordinate system then Gtk/Gdk.
As of upload left PhxObject code in example of Gtk linkage of lci_findport.
