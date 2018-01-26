/*
 * fault_handler.cpp
 *
 *  Created on: Jan 25, 2018
 *      Author: josef
 */
#include "fault_handler.h"
#include "../intercept/cpu_session.h"

using namespace Rtcr;


Managed_region_map_info *Fault_handler::_find_faulting_mrm_info()
{
	Genode::Region_map::State state;
	Managed_region_map_info *result_info = nullptr;

	Ram_dataspace_info *ramds_info = _ramds_infos.first();
	while(ramds_info && !result_info)
	{
		if(!ramds_info->mrm_info)
			continue;

		Genode::Region_map_client rm_client(ramds_info->mrm_info->region_map_cap);

		if(rm_client.state().type != Genode::Region_map::State::READY)
		{
			result_info = ramds_info->mrm_info;
			break;
		}

		ramds_info = ramds_info->next();
	}

	return result_info;
}


void Fault_handler::_handle_fault()
{
	// Find faulting Managed_region_info
	Managed_region_map_info *faulting_mrm_info = _find_faulting_mrm_info();


	// Get state of faulting Region_map
	Genode::Region_map::State state = Genode::Region_map_client{faulting_mrm_info->region_map_cap}.state();

	if(verbose_debug)
	{
	Genode::log("Handle fault: Region map ",
			faulting_mrm_info->region_map_cap, " state is ",
			state.type == Genode::Region_map::State::READ_FAULT  ? "READ_FAULT"  :
			state.type == Genode::Region_map::State::WRITE_FAULT ? "WRITE_FAULT" :
			state.type == Genode::Region_map::State::EXEC_FAULT  ? "EXEC_FAULT"  : "READY",
			" pf_addr=", Genode::Hex(state.addr), " ip: ", Genode::Hex(state.pf_ip));
	}

	// Find dataspace which contains the faulting address
	Designated_dataspace_info *dd_info = faulting_mrm_info->dd_infos.first();
	if(dd_info) dd_info = dd_info->find_by_addr(state.addr);

	// Check if a dataspace was found
	if(!dd_info)
	{
		Genode::warning("No designated dataspace for addr = ", state.addr,
				" in Region_map ", faulting_mrm_info->region_map_cap);
		return;
	}




	Cpu_session_component* c = Cpu_session_component::current_session;


	while (c) {

		// Iterate through every CPU thread
		Cpu_thread_component *cpu_thread =
				c->parent_state().cpu_threads.first();
		int j = 0;
		while (cpu_thread) {
			// Pause the CPU thread
			//Genode::Cpu_thread_client client{cpu_thread->parent_cap()};
			//client.pause();
			//if(cpu_thread->state().unresolved_page_fault)
			{
				if(state.pf_ip == cpu_thread->state().ip)
					PINF("Page-faulting Thread: %i, Pagefault: %i, IP: %lx", j,
						cpu_thread->state().unresolved_page_fault,
						cpu_thread->state().ip);
//				Genode::Thread_state st = cpu_thread->state();
				//st.ip += 2;
//				cpu_thread->state(st);
				j++;

			}
			cpu_thread = cpu_thread->next();
		}

		c = c->next();
	}










	// Don't attach found dataspace to its designated address,
	// because we need to continue receiving pagefaults in order
	// to simulate them
	//dd_info->attach();
	//dd_info->detach();

	//TODO: Resolve pagefault by simulating access


	//TODO: Increase instruction pointer (ip) by one word
	//Genode::Region_map_client::State client_state = Genode::Region_map_client{faulting_mrm_info->region_map_cap}.state();
	//Genode::Thread_state::Cpu_state* ts = static_cast<Genode::Thread_state::Cpu_state*>(&client_state);
	//ts->ip++;
	//faulting_mrm_info->

	//Rm_session_component bla;
	//bla.find_by_badge(2345);

	//Genode::Region_map_client rm_cli =	Genode::Region_map_client{faulting_mrm_info->region_map_cap};


	//continue execution since we resolved the pagefault
	//Genode::Region_map_client{faulting_mrm_info->region_map_cap}.processed(state);
	//dd_info->
}


Fault_handler::Fault_handler(Genode::Env &env, Genode::Signal_receiver &receiver,
		Genode::List<Ram_dataspace_info> &ramds_infos)
:
	Thread(env, "managed dataspace pager", 16*1024),
	_receiver(receiver), _ramds_infos(ramds_infos)
{ }


void Fault_handler::entry()
{
	while(true)
	{
		Genode::Signal signal = _receiver.wait_for_signal();
		for(unsigned int i = 0; i < signal.num(); ++i)
			_handle_fault();
	}
}
