/*
 * \brief  Rtcr child creation
 * \author Denis Huber
 * \date   2016-08-26
 */

/* Genode include */
#include <base/component.h>
#include <base/signal.h>
#include <base/sleep.h>
#include <base/log.h>
#include <base/child.h>
//#include <base/internal/elf.h>
#include <../src/include/base/internal/elf.h>
//#include <../src/include/base/internal/elf_format.h>
#include <timer_session/connection.h>

/* Rtcr includes */
#include "../../rtcr/target_child.h"
#include "../../rtcr/target_state.h"
#include "../../rtcr/checkpointer.h"
#include "../../rtcr/restorer.h"
#include "../../rtcr/util/simplified_managed_dataspace_info.h"

namespace Rtcr {
struct Main;
}

struct Rtcr::Main {
	enum {
		ROOT_STACK_SIZE = 16 * 1024
	};
	Genode::Env &env;
	Genode::Heap heap { env.ram(), env.rm() };
	Genode::Service_registry parent_services { };

	Main(Genode::Env &env_) :
			env(env_) {
		using namespace Genode;

		Timer::Connection timer { env };
		size_t time_start;
		size_t time_end;

		//const Genode::size_t granularity = 0;
		//const Genode::size_t granularity = 1;
		const Genode::size_t granularity = Target_child::GRANULARITY_REDUNDANT_MEMORY;

		Target_child* child = new (heap) Target_child { env, heap, parent_services, "sheep_counter", granularity };
		//Target_state ts(env, heap, false);
		Target_state ts(env, heap, true);
		Checkpointer ckpt(heap, *child, ts);
		child->start();

		timer.msleep(1000);
		ckpt.set_redundant_memory(true);

		for (int i = 0; i < 2 ; i++) {

			timer.msleep(3000);

			time_start = timer.elapsed_ms();
			ckpt.checkpoint();
			time_end = timer.elapsed_ms();
			PINF("Time for checkpointing: %ums", time_end-time_start);
		}
		timer.msleep(2000);

		ckpt.set_redundant_memory(false);

		timer.msleep(2000);

		child->pause();
		timer.msleep(1000);
		Target_child* child_restored = new (heap) Target_child { env, heap, parent_services, "sheep_counter", granularity };

		Restorer resto(heap, *child_restored, ts);
		//child_restored->start(resto);

		Target_state ts_restored(env, heap, true);
		Checkpointer ckpt_restored(heap, *child_restored, ts_restored);

		child_restored->start();

		timer.msleep(3000);
		//ckpt_restored.set_redundant_memory(true);


		// Manual memory-only restore

		/* Find the custom dataspace info with the snapshot inside:
		 * It's the ds that sheep_counter created last during runtime,
		 * so it's the last one in the list.
		 */
		Designated_redundant_ds_info* src_drdsi = nullptr;
		for(auto src_smdi = ts._managed_redundant_dataspaces.first(); src_smdi != nullptr; src_smdi = src_smdi->next())
			for(auto src_sddsi = src_smdi->designated_dataspaces.first(); src_sddsi != nullptr; src_sddsi = src_sddsi->next())
				src_drdsi = src_sddsi->redundant_memory;
		/* Now, locate the target ds of the new task.
		 * The order of ram_dataspaces is reversed,
		 * so this time we need the first element.
		 */
		Ram_dataspace_info* dst_rdsi = child_restored->ram().parent_state().ram_dataspaces.first();
		Designated_dataspace_info* dst_ddsi = dst_rdsi->mrm_info->dd_infos.first();
		addr_t primary_ds_loc_addr = env.rm().attach(dst_ddsi->cap);
		// Now we can do the memory restoration:
		src_drdsi->copy_from_latest_checkpoint((void*)primary_ds_loc_addr);
		env.rm().detach(primary_ds_loc_addr);
		PINF("MEMORY RESTORED!");

		timer.msleep(5000);

		log("The End.");
		Genode::sleep_forever();
	}
};

Genode::size_t Component::stack_size() {
	return 32 * 1024;
}

void Component::construct(Genode::Env &env) {
	static Rtcr::Main main(env);
}
