TEMPLATE = subdirs

SUBDIRS += \
    utils \
    spc_ex \
    stx_ex

spc_ex.depends = utils
stx_ex.depends = utils
