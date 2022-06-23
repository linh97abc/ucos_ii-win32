UCOSII_SRC := os_core.c  os_flag.c  os_mem.c    os_q.c    os_task.c  os_tmr.c\
	os_dbg.c   os_mbox.c  os_mutex.c  os_sem.c  os_time.c

src_$(CONFIG_UCOSII) += $(UCOSII_SRC:%=$(UCOSII_BASE)/src/%)
