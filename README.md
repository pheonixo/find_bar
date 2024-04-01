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

  1 April 24. Major altercations which caused additional code. Banks, simular
to combo popups, menus, and treeviews, reworked. Text based objects reworked to
include utf8 coding/display. Utf8 forced the additional libctype and ext_cairo.c
code. This may cause you to ether download Noto fonts or alter code for your
system's font coverage of other script glyphs. It adds dependancy of Freetype,
but avoids fontconfig. Added in rework of label objects such that justification
and resizing of multi-line text occurs within the alloted area (mete_box).
Also added code to dynamically shift allocation of objects from expected 256 or
less objects, to the max of 65535 objects.
