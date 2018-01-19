TEMPLATE = subdirs

SUBDIRS += \
    utils \
    spc_ex \
    stx_ex \
    stx_editor \
    spc_editor

spc_ex.depends = utils
stx_ex.depends = utils
stx_editor.depends = utils
spc_editor.depends = utils
