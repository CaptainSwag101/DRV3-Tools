TEMPLATE = subdirs

SUBDIRS += \
    utils \
    unit_tests \
    spc_ex \
    stx_ex \
    spc_editor \
    stx_editor \
    wrd_editor \
    dat_editor


unit_tests.depends = utils
spc_ex.depends = utils
stx_ex.depends = utils
spc_editor.depends = utils
stx_editor.depends = utils
wrd_editor.depends = utils
dat_editor.depends = utils
