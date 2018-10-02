#
# Makefile for sensorHub drivers.
#
ifneq (, $(filter F%, $(SH_BUILD_ID)))
EXTRA_CFLAGS += -DSHUB_SW_FACTORY_MODE
endif

ifeq ($(CONFIG_SHUB_ML630Q790), y)
obj-y += ml630q790_lib.o

ml630q790_lib-y := ml630q790.o \
                   shub-batching.o \
                   shub-input_mcu.o \
                   shub-input_exif.o \
                   shub-input_common.o \
                   shub-spi.o \
                   shub-input_acc.o \
                   shub-input_gyro.o \
                   shub-input_mag.o \
                   shub-input_orientation.o \
                   shub-input_linear_acc.o \
                   shub-input_game_rotation_vec.o \
                   shub-input_grav.o \
                   shub-input_gyro_uncal.o \
                   shub-input_mag_rotation_vec.o \
                   shub-input_mag_uncalib.o \
                   shub-input_rotation_vec.o \
                   shub-input_pedo.o \
                   shub-input_pedodect.o \
                   shub-input_significant.o \
                   shub-input_device_orientation.o \
                   shub-input_diag.o
endif

EXTRA_CFLAGS += -Wall

