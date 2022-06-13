#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "twe_common.hpp"
#include <memory>

namespace TWE {

	/**
	 * @class	APP_DEF
	 *
	 * @brief	An application definition having setup(), loop() function.
	 */
	class APP_DEF {
		void* _pobj;
	public:
		APP_DEF() : _pobj(nullptr) {}

		virtual void setup() = 0;
		virtual void loop() = 0;
		virtual ~APP_DEF() {}

		void set_appobj(void* p) {
			_pobj = p;
		}

		void* get_appobj() {
			return _pobj;
		}
	};

	/**
	 * @class	APP_MGR
	 *
	 * @brief	Manager for sub application having setup(), loop() function.
	 */
	class APP_MGR {
	public:

		static const int NEXT_APP_DEFAULT = -1;

		/**
		 * @typedef	int (*PF_CHANGE_APP)( APP_MGR& app, int next_app, int prev_app, int exit_code )
		 *
		 * @brief	APP_DEF object switcher called from APP_MGR::loop().
		 * 			This function is implemented by user and registered by APP_MGR::setup() method.
		 * 			
		 * @param	app			APP_MGR object itself
		 * 						
		 * @param	next_app	Requested app_id for next APP_DEF object.
		 * 						-1: APP_MGR::exit() called, The switcher function should switch proper
		 * 						    APP_DEF object (e.g. root menu app for application select)
		 * 						0: no app_id specified,
		 * 						1..n: Specify application by unique app_id.    
		 * 						
		 * @param   prev_app    The app id about to exit. It could be set 0 at the first change or
		 * 						having some error.
		 * 						
		 * @param   exit_code   Pass the 1st parameter of APP_DEF::exit(). For example, the implemented
		 * 						switcher function can chose the next APP_DEF object by using exit code,
		 * 						instead of using APP_DEF::set_nextapp().
		 * 						
		 * @returns				The generated app_id if 1..n, otherwise app has not been switched.
		 */
		typedef int (*PF_CHANGE_APP)(
			APP_MGR& app,
			int next_app,
			int prev_app,
			int exit_code
		);

	private:
		std::unique_ptr<APP_DEF> _the_app;
		int _next_app;
		int _exit_code;
		int _app_sel;
		PF_CHANGE_APP _p_change_app;

	public:
		APP_MGR() 
			: _the_app()
			, _next_app(0)
			, _exit_code(0)
			, _app_sel(0)
			, _p_change_app(nullptr)
		{
			;
		}

		/**
		 * @fn	void APP_MGR::setup(PF_CHANGE_APP chapp)
		 *
		 * @brief	initial setup.
		 *
		 * @param	chapp	Function pointer to app switcher function.
		 */
		void setup(PF_CHANGE_APP chapp) {
			_p_change_app = chapp;
		}

		/**
		 * @fn	template <class T, class... Args> void APP_MGR::new_app(Args&&... args)
		 *
		 * @brief	Creates a new application. The APP_DEF object is constructed
		 * 			with variable arguments passed by template parameter pack.
		 *
		 * @tparam	T   	class of APP_DEF object
		 * @tparam	Args	Constructor parameter pack
		 * @param	args	Variable argument for constructor
		 */
		template <class T, class... Args>
		void new_app(Args&&... args) {
			_the_app.reset(new T(std::forward<Args&&>(args)...));
			_the_app->setup();
			return;
		}

		/**
		 * @fn	void APP_MGR::loop()
		 *
		 * @brief	called from main loop().
		 */
		void loop() {
			if (_the_app) {
				_the_app->loop();
			}
			if (_next_app != 0) {
				if (_p_change_app)
					_app_sel = _p_change_app(*this, _next_app, _app_sel, _exit_code);
				else
					_app_sel = 0;

				_next_app = 0;
			}
		}

		/**
		 * @fn	void APP_MGR::exit(int exit_code)
		 *
		 * @brief	request to exit the app, called from APP_DEF::loop(), 
		 *
		 * @param	exit_code	The exit code.
		 */
		void exit(int exit_code, int next_app = NEXT_APP_DEFAULT) {
			_next_app = next_app;
			_exit_code = exit_code;
		}

		/**
		 * @fn	operator APP_MGR::bool()
		 *
		 * @brief	check the app is registered or not.
		 *
		 * @returns	true: app is registered.
		 */
		explicit operator bool() {
			return _the_app.operator bool();
		}

		/**
		 * @fn	APP_DEF* APP_MGR::query_appobj()
		 *
		 * @brief	Queries the appobj
		 *
		 * @returns	Null if it fails, else the appobj.
		 */
		APP_DEF* query_appobj() {
			return _the_app.get();
		}

		/**
		 * clean up the instance.
		 */
		void _destroy_app_instance() {
			_the_app.reset(nullptr);
		}
	};

	extern class APP_MGR the_app;


	/**
	 * @struct	APP_HANDLR_DC
	 *
	 * @brief	An application handlr device-context.
	 */
	struct APP_HANDLR_DC {
		const int _class_id;
		APP_HANDLR_DC(int id) : _class_id(id) {}
		virtual ~APP_HANDLR_DC() {} // must be virtual, when deleting an instance inherited from APP_HANDLR_DC.
	};

	/**
	 * @class	APP_HNDLR
	 *
	 * @brief	CRTP super class proving very simple state handling,
	 * 			just switching setup/loop function by new_hndlr().
	 * 			
	 *          class c_foo : public APP_DEF, public APP_HNDLR<c_foo> {
	 *          protected:
	 *				void state_alpha(bool b_setup) {
	 *					if (b_setup) {
	 *                     ; // when new_hndlr() is called.
	 *                  } else {
	 *                     ; // when APP_HNDLR::loop() is called.
	 *                     if (...) { // new state
	 *					       APP_HNDLR::new_hndlr(&c_foo::state_bravo);
	 *					   }
	 *                  }
	 *				}
	 *				
	 *              void state_bravo(bool b_setup) { ... }
	 *          public:
	 *				void setup() { // called from APP_DEF management class, run once at the init.
	 *				  APP_HNDLR::new_hndlr(&c_foo::state_alpha); // init state
	 *				}
	 *				void loop() { // called from APP_DEF management class, looping.
	 *				  APP_HNDLR::loop();
	 *				}
	 *          }
	 *
	 * @tparam	T	Generic type parameter.
	 */
	template <class T>
	class APP_HNDLR {
	public:
		typedef uint32_t event_type;
		typedef uint32_t arg_type;

		static const event_type EV_SETUP = 0;
		static const event_type EV_LOOP = 1;
		static const event_type EV_EXIT = 2;

		typedef void (T::* tpf_func_handler)(event_type ev, arg_type evarg);

	private:
		tpf_func_handler _pf;
		std::unique_ptr<APP_HANDLR_DC> _dc;

	public:
		APP_HNDLR() : _pf(nullptr) {}
		~APP_HNDLR() {
			// here, calling _pf() may cause a problem.
		}


		/**
		 * @fn	void APP_HNDLR::on_close()
		 *
		 * @brief	Handles close signals.
		 * 			if EV_EXIT call is mandate for handlers on destructing a class inherits APP_HNDLR,
		 * 			call this on the class destructor.
		 */
		void on_close() {
			if (_pf) {
				auto derived = static_cast<T*>(this);
				((*derived).*(_pf))(EV_EXIT, 0);
				_pf = nullptr;
			}

			if (_dc) {
				_dc.reset();
			}
		}


		/**
		 * @fn	void APP_HNDLR::new_hndlr(tpf_func_handler hnd_next, arg_type arg = 0)
		 *
		 * @brief	Switch to the new handler. The old one is destructed here.
		 *
		 * @param	hnd_next	The handle next.
		 * @param	arg			(Optional) The argument.
		 */
		void new_hndlr(tpf_func_handler hnd_next, arg_type arg = 0) {
			auto derived = static_cast<T*>(this);

			// exit last event handler
			if (_pf) {
				((*derived).*(_pf))(EV_EXIT, 0);
				_pf = nullptr;
				
				if (_dc) _dc.reset();
			}

			_pf = hnd_next;

			if (_pf) {
				((*derived).*(_pf))(EV_SETUP, arg);
			}
		}


		/**
		 * @fn	void APP_HNDLR::loop(arg_type arg = 0)
		 *
		 * @brief	Loops the given argument
		 *
		 * @param	arg	(Optional) The argument.
		 */
		void loop(arg_type arg = 0) {
			if (_pf) {
				auto derived = static_cast<T*>(this);
				((*derived).*(_pf))(EV_LOOP, arg);
			}
		}


		/**
		 * @fn	template <class DC> DC& APP_HNDLR::use()
		 *
		 * @brief	Prepare data context (dc) for each handlers.
		 *
		 * @tparam	DC	Structure type derived from APP_HANDLR_DC, which contains dedicated data structure/procedure.
		 * 				- This class must have int DC::CLS_ID as public scope.  
		 * 				- Recommended to be friend class to sub-app (derived class of APP_DEF)
		 *
		 * @returns	A reference to the data context
		 */
		template <class DC, class... Args>
		DC& use(Args&&... args) {
			if (_dc && _dc->_class_id == DC::CLS_ID) {
				// existing object
				return *static_cast<DC*>(_dc.get());
			}
			else {
				// create new DC instance (if obj class id differs)
				T* papp = static_cast<T*>(this);
				DC* pdc = new DC(*papp, std::forward<Args&&>(args)...);
				_dc.reset(pdc);
				return *pdc;
			}
		}
	};
}

/** 
 * template code of handlers for APP_HNDLR.
 */
#define EMBED_APP_HNDLR_TEMPLATE_PROCEDURE(FNAME) \
template <typename T> \
void FNAME(event_type ev, arg_type arg) { \
	auto&& dc = APP_HNDLR::use<T>(); \
	switch (ev) { \
	case EV_SETUP: dc.setup(); break; \
	case EV_LOOP: dc.loop(); break; \
	case EV_EXIT: dc.on_close(); break; \
	} \
}

