/*
 * \brief  Child creation
 * \author Denis Huber
 * \date   2016-08-04
 */

#include "target_child.h"

using namespace Rtcr;


Target_child::Resources::Resources(Genode::Env &env, Genode::Entrypoint &ep, Genode::Allocator &md_alloc,
		const char *name, Genode::size_t granularity)
:
	ep  (ep),
	pd  (env, md_alloc, ep, name),
	cpu (env, md_alloc, ep, pd.parent_cap(), name),
	ram (env, md_alloc, ep, name, granularity),
	rom (env, name)
{
	ep.manage(pd);
	ep.manage(cpu);
	ep.manage(ram);

	if(verbose_debug)
	{
		Genode::log("Managing PD  session ", pd.cap());
		Genode::log("Managing CPU session ", cpu.cap());
		Genode::log("Managing RAM session ", ram.cap());
	}


	// Donate ram quota to child
	// TODO Replace static quota donation with the amount of quota, the child needs
	Genode::size_t donate_quota = 1024*1024;
	ram.ref_account(env.ram_session_cap());
	// Note: transfer goes directly to parent's ram session
	env.ram().transfer_quota(ram.parent_cap(), donate_quota);
}


Target_child::Resources::~Resources()
{
	ep.dissolve(ram);
	ep.dissolve(cpu);
	ep.dissolve(pd);

	if(verbose_debug)
	{
		Genode::log("Dissolving PD  session ", pd.cap());
		Genode::log("Dissolving CPU session ", cpu.cap());
		Genode::log("Dissolving RAM session ", ram.cap());
	}
}


void Target_child::_restore()
{
	if(verbose_debug) Genode::log("\033[33m", __func__, "\033[0m()");

	if(!_copy)
	{
		Genode::log("No valid copy object!");
		return;
	}


}


Target_child::Target_child(Genode::Env &env, Genode::Allocator &md_alloc,
		Genode::Service_registry &parent_services, const char *name, Genode::size_t granularity)
:
	_name            (name),
	_env             (env),
	_md_alloc        (md_alloc),
	_resources_ep    (_env, 16*1024, "resources ep"),
	_child_ep        (_env, 16*1024, "child ep"),
	_granularity     (granularity),
	_phase_restore   (false),
	_copy            (nullptr),
	_resources       (_env, _resources_ep, _md_alloc, _name.string(), _granularity),
	_initial_thread  (_resources.cpu, _resources.pd.cap(), _name.string()),
	_address_space   (_resources.pd.address_space()),
	_parent_services (parent_services),
	_local_services  (),
	_child_services  (),
	_child           (nullptr)
{
	if(verbose_debug) Genode::log("\033[33m", __func__, "\033[0m(child=", _name.string(), ")");
}

Target_child::~Target_child()
{
	if(_child)
		Genode::destroy(_md_alloc, _child);

	if(verbose_debug) Genode::log("\033[33m", __func__, "\033[0m() ", _name.string());
}

void Target_child::start()
{
	if(verbose_debug) Genode::log("Target_child::\033[33m", __func__, "\033[0m()");

	_phase_restore = false;

	_child = new (_md_alloc) Genode::Child(
			_resources.rom.dataspace(), Genode::Dataspace_capability(),
			_resources.pd.cap(),  _resources.pd,
			_resources.ram.cap(), _resources.ram,
			_resources.cpu.cap(), _initial_thread,
			_env.rm(), _address_space, _child_ep.rpc_ep(), *this);


}

void Target_child::start(Target_copy &copy)
{
	if(verbose_debug) Genode::log("Target_child::\033[33m", __func__, "\033[0m(from_copy=", &copy,")");

	_phase_restore = true;
	_copy = &copy;

	_child = new (_md_alloc) Genode::Child(
			_resources.rom.dataspace(), Genode::Dataspace_capability(),
			_resources.pd.cap(),  _resources.pd,
			_resources.ram.cap(), _resources.ram,
			_resources.cpu.cap(), _initial_thread,
			_env.rm(), _address_space, _child_ep.rpc_ep(), *this);


}

Genode::Service *Target_child::resolve_session_request(const char *service_name, const char *args)
{
	if(verbose_debug) Genode::log("Target_child::\033[33m", __func__, "\033[0m(", service_name, " ", args, ")");

	// Restoration hook
	if(!Genode::strcmp(service_name, "LOG") && _phase_restore)
	{
		_restore();

		_phase_restore = false;
	}

	// TODO Support grandchildren: PD, CPU, and RAM session has also to be provided to them

	Genode::Service *service = 0;

	// Service is implemented locally?
	service = _local_services.find(service_name);
	if(service)
		return service;

	// Service known from parent?
	service = _parent_services.find(service_name);
	if(service)
		return service;

	// Service known from child?
	service = _child_services.find(service_name);
	if(service)
		return service;

	// Intercept these sessions
	if(!Genode::strcmp(service_name, "RM"))
	{
		Rm_root *root = new (_md_alloc) Rm_root(_env, _md_alloc, _resources_ep);
		service = new (_md_alloc) Genode::Local_service(service_name, root);
		_local_services.insert(service);
		if(verbose_debug) Genode::log("  inserted service into local_services");
	}
	else if(!Genode::strcmp(service_name, "LOG"))
	{
		Log_root *root = new (_md_alloc) Log_root(_env, _md_alloc, _resources_ep);
		service = new (_md_alloc) Genode::Local_service(service_name, root);
		_local_services.insert(service);
		if(verbose_debug) Genode::log("  inserted service into local_services");
	}
	else if(!Genode::strcmp(service_name, "Timer"))
	{
		Timer_root *root = new (_md_alloc) Timer_root(_env, _md_alloc, _resources_ep);
		service = new (_md_alloc) Genode::Local_service(service_name, root);
		_local_services.insert(service);
		if(verbose_debug) Genode::log("  inserted service into local_services");
	}

	// Service not known, cannot intercept its methods
	if(!service)
	{
		service = new (_md_alloc) Genode::Parent_service(service_name);
		_parent_services.insert(service);
		Genode::warning("Unknown service: ", service_name);
	}

	return service;
}


void Target_child::filter_session_args(const char *service, char *args, Genode::size_t args_len)
{
	Genode::Arg_string::set_arg_string(args, args_len, "label", _name.string());
}
