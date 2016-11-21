/*
 * \brief  Structure for storing thread information
 * \author Denis Huber
 * \date   2016-10-06
 */

#ifndef _RTCR_STORED_THREAD_INFO_H_
#define _RTCR_STORED_THREAD_INFO_H_

/* Genode includes */
#include <util/list.h>

/* Rtcr includes */
#include "../monitor/cpu_thread_info.h"

namespace Rtcr {
	struct Stored_thread_info;
}


struct Rtcr::Stored_thread_info : Genode::List<Stored_thread_info>::Element
{
	Genode::addr_t              kcap;
	Genode::uint16_t            badge;
	bool                        started;
	bool                        paused;
	Genode::uint16_t            exception_sigh_badge;
	bool                        single_step;
	Genode::Cpu_session::Name   name;
	Genode::Affinity::Location  affinity;
	Genode::Cpu_session::Weight weight;
	Genode::addr_t              utcb;
	Genode::Thread_state        ts;

	Stored_thread_info()
	:
		kcap(0), badge(0), started(false), paused(false), exception_sigh_badge(0),
		single_step(false), name(), affinity(), weight(), utcb(0), ts()
	{ }

	Stored_thread_info(Thread_info &info)
	:
		kcap(0), badge(info.cpu_thread.cap().local_name()),
		started(info.cpu_thread.parent_state().started),
		paused(info.cpu_thread.parent_state().paused),
		exception_sigh_badge(info.cpu_thread.parent_state().exception_sigh.local_name()),
		single_step(info.cpu_thread.parent_state().single_step),
		name(info.name),
		affinity(info.cpu_thread.parent_state().affinity),
		weight(info.weight), utcb(info.utcb), ts()
	{ }

	Stored_thread_info *find_by_name(const char *name)
	{
		if(!Genode::strcmp(name, this->name.string()))
			return this;
		Stored_thread_info *info = next();
		return info ? info->find_by_name(name) : 0;
	}

	Stored_thread_info *find_by_badge(Genode::uint16_t badge)
	{
		if(badge == this->badge)
			return this;
		Stored_thread_info *info = next();
		return info ? info->find_by_badge(badge) : 0;
	}

	void print(Genode::Output &output) const
	{
		using Genode::Hex;

		Genode::print(output, "<", Hex(kcap), ", ", badge, "> started=", started?"t":"f",
				" paused=", paused?"t":"f", " single_step=", single_step?"t":"f",
				" exception_sigh_badge=", exception_sigh_badge?"t":"f",
				" affinity=(", affinity.xpos(), "x", affinity.ypos(),
				", ", affinity.width(), "x", affinity.height(), ")",
				" weight=", weight.value, "\n");
		Genode::print(output, "  Thread ", name.string(), "\n");
		Genode::print(output, "  r0-r4: ", Hex(ts.r0, Hex::PREFIX, Hex::PAD), " ",
				Hex(ts.r1, Hex::PREFIX, Hex::PAD), " ", Hex(ts.r2, Hex::PREFIX, Hex::PAD), " ",
				Hex(ts.r3, Hex::PREFIX, Hex::PAD), " ", Hex(ts.r4, Hex::PREFIX, Hex::PAD), "\n");
		Genode::print(output, "  r5-r9: ", Hex(ts.r5, Hex::PREFIX, Hex::PAD), " ",
				Hex(ts.r6, Hex::PREFIX, Hex::PAD), " ", Hex(ts.r7, Hex::PREFIX, Hex::PAD), " ",
				Hex(ts.r8, Hex::PREFIX, Hex::PAD), " ", Hex(ts.r9, Hex::PREFIX, Hex::PAD), "\n");
		Genode::print(output, "  r10-r12: ", Hex(ts.r10, Hex::PREFIX, Hex::PAD), " ",
				Hex(ts.r11, Hex::PREFIX, Hex::PAD), " ", Hex(ts.r12, Hex::PREFIX, Hex::PAD), "\n");
		Genode::print(output, "  sp, lr, ip, cpsr, cpu_e: ", Hex(ts.sp, Hex::PREFIX, Hex::PAD), " ",
				Hex(ts.lr, Hex::PREFIX, Hex::PAD), " ", Hex(ts.ip, Hex::PREFIX, Hex::PAD), " ",
				Hex(ts.cpsr, Hex::PREFIX, Hex::PAD), " ", Hex(ts.cpu_exception, Hex::PREFIX, Hex::PAD));
	}

};

#endif /* _RTCR_STORED_THREAD_INFO_H_ */
