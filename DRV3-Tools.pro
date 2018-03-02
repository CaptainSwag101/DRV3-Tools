TEMPLATE = subdirs

SUBDIRS += \
    utils \
    spc_ex \
    stx_ex \
    stx_editor \
    spc_editor \
    wrd_editor \
    unit_tests

spc_ex.depends = utils
stx_ex.depends = utils
stx_editor.depends = utils
spc_editor.depends = utils
wrd_editor.depends = utils
unit_tests.depends = utils
