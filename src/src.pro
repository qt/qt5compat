TEMPLATE = subdirs

SUBDIRS += \
    core5

qtHaveModule(gui):qtHaveModule(quick): \
    SUBDIRS += imports
