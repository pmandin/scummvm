MODULE := engines/nancy

MODULE_OBJS = \
  action/actionmanager.o \
  action/actionrecord.o \
  action/arfactory.o \
  action/bombpuzzle.o \
  action/collisionpuzzle.o \
  action/conversation.o \
  action/leverpuzzle.o \
  action/orderingpuzzle.o \
  action/overlay.o \
  action/overridelockpuzzle.o \
  action/passwordpuzzle.o \
  action/raycastpuzzle.o \
  action/recordtypes.o \
  action/riddlepuzzle.o \
  action/rippedletterpuzzle.o \
  action/rotatinglockpuzzle.o \
  action/safelockpuzzle.o \
  action/secondarymovie.o \
  action/secondaryvideo.o \
  action/setplayerclock.o \
  action/sliderpuzzle.o \
  action/soundequalizerpuzzle.o \
  action/tangrampuzzle.o \
  action/towerpuzzle.o \
  action/turningpuzzle.o \
  action/telephone.o \
  ui/fullscreenimage.o \
  ui/animatedbutton.o \
  ui/button.o \
  ui/clock.o \
  ui/inventorybox.o \
  ui/ornaments.o \
  ui/scrollbar.o \
  ui/textbox.o \
  ui/viewport.o \
  state/credits.o \
  state/logo.o \
  state/loadsave.o \
  state/help.o \
  state/mainmenu.o \
  state/map.o \
  state/savedialog.o \
  state/scene.o \
  state/setupmenu.o \
  misc/lightning.o \
  misc/specialeffect.o \
  commontypes.o \
  console.o \
  cursor.o \
  decompress.o \
  enginedata.o \
  font.o \
  graphics.o \
  iff.o \
  input.o \
  metaengine.o \
  nancy.o \
  puzzledata.o \
  renderobject.o \
  resource.o \
  sound.o \
  util.o \
  video.o

# This module can be built as a plugin
ifeq ($(ENABLE_NANCY), DYNAMIC_PLUGIN)
PLUGIN := 1
endif

# Include common rules
include $(srcdir)/rules.mk

# Detection objects
DETECT_OBJS += $(MODULE)/detection.o
