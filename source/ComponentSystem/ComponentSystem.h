/* Copyright (C) 2005-2020, UNIGINE. All rights reserved.
 *
 * This file is a part of the UNIGINE 2.11.0.1 SDK.
 *
 * Your use and / or redistribution of this software in source and / or
 * binary form, with or without modification, is subject to: (i) your
 * ongoing acceptance of and compliance with the terms and conditions of
 * the UNIGINE License Agreement; and (ii) your inclusion of this notice
 * in any version of this software that you use or redistribute.
 * A copy of the UNIGINE License Agreement is available by contacting
 * UNIGINE. at http://unigine.com/
 */


#pragma once
#include <UnigineNode.h>
#include <UnigineMap.h>
#include <UnigineLogic.h>
#include <UnigineProperties.h>
#include <UnigineMaterials.h>
#include <UnigineFileSystem.h>
#include <UnigineCallback.h>
#include <UnigineThread.h>
#include <UnigineDir.h>

// adds to MicroProfiler full info about components
// #define USE_MICROPROFILE

class ComponentBase;

////////////////////////////////////////////////////////////////////////////////////////
// Component Macros
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __GNUC__
#pragma region Macros
#endif

// component
#define COMPONENT(CLASS_NAME, PARENT_NAME)												\
	public:																				\
		CLASS_NAME(const Unigine::NodePtr &node, int num) : PARENT_NAME(node, num) {}	\
		virtual ~CLASS_NAME() {}														\
		using __this_class = CLASS_NAME;												\
		const char *getClassName() override { return #CLASS_NAME; }

// property
#define PROP_NAME(NAME) static const char *getPropertyName() { return NAME; }
#define PROP_PARENT_NAME(PARENT_NAME) const char *getParentPropertyName() override { return PARENT_NAME; }
#define PROP_AUTOSAVE(VALUE) int isAutoSaveProperty() override { return VALUE; }

// parameters
#define PROP_PARAM(TYPE, NAME, ...) \
	ComponentVariable##TYPE NAME { this, #NAME, ##__VA_ARGS__ };
#define PROP_STRUCT(TYPE, NAME, ...) \
	ComponentVariableStruct<TYPE> NAME { this, #TYPE, #NAME, ##__VA_ARGS__ };
#define PROP_ARRAY(TYPE, NAME, ...) \
	ComponentVariableArray<ComponentVariable##TYPE> NAME { this, #NAME, #TYPE, ##__VA_ARGS__ };
#define PROP_ARRAY_STRUCT(TYPE, NAME, ...) \
	ComponentVariableArray<ComponentVariableStruct<TYPE>> NAME { this, #NAME, #TYPE, ##__VA_ARGS__ };

// register methods
#define COMPONENT_METHOD(TYPE, NAME, ...)																			\
	template <typename T>																							\
	class ComponentMethodRegistrator_##NAME																			\
	{																												\
	public:																											\
		ComponentBase *component;																					\
		Unigine::CallbackBase *func;																				\
		Unigine::String name;																						\
		ComponentMethodRegistrator_##NAME(T *c)																		\
		{																											\
			component = c;																							\
			func = Unigine::MakeCallback(c, &T::NAME);																\
			name = Unigine::String::format("%s::%s", c->getClassName(), #NAME);										\
			ComponentSystem::get()->addComponentMethod##TYPE(component, func, name.get(), #NAME, ##__VA_ARGS__);	\
		}																											\
		~ComponentMethodRegistrator_##NAME()																		\
		{																											\
			ComponentSystem::get()->removeComponentMethod##TYPE(component, func, name.get(), #NAME, ##__VA_ARGS__);	\
			delete func;																							\
		}																											\
	};																												\
	ComponentMethodRegistrator_##NAME<__this_class> __method_registrator_##NAME{ this };

#define COMPONENT_INIT(NAME, ...)						COMPONENT_METHOD(Init, NAME, ##__VA_ARGS__)
#define COMPONENT_UPDATE_ASYNC_THREAD(NAME, ...)		COMPONENT_METHOD(UpdateAsyncThread, NAME, ##__VA_ARGS__)
#define COMPONENT_UPDATE_SYNC_THREAD(NAME, ...)			COMPONENT_METHOD(UpdateSyncThread, NAME, ##__VA_ARGS__)
#define COMPONENT_UPDATE(NAME, ...)						COMPONENT_METHOD(Update, NAME, ##__VA_ARGS__)
#define COMPONENT_POST_UPDATE(NAME, ...)				COMPONENT_METHOD(PostUpdate, NAME, ##__VA_ARGS__)
#define COMPONENT_UPDATE_PHYSICS(NAME, ...)				COMPONENT_METHOD(UpdatePhysics, NAME, ##__VA_ARGS__)
#define COMPONENT_SWAP(NAME, ...)						COMPONENT_METHOD(Swap, NAME, ##__VA_ARGS__)
#define COMPONENT_SHUTDOWN(NAME, ...)					COMPONENT_METHOD(Shutdown, NAME, ##__VA_ARGS__)
#define COMPONENT_DESTROY_RENDER_RESOURCES(NAME, ...)	COMPONENT_METHOD(DestroyRenderResources, NAME, ##__VA_ARGS__)

// register component in the ComponentSystem
// you can use this macro instead of using ComponentSystem::get()->registerComponent<CLASS_NAME>();
// put it in your .cpp file
#define REGISTER_COMPONENT(CLASS_NAME)																									\
class ComponentRegistrator_##CLASS_NAME																									\
{																																		\
public:																																	\
	ComponentRegistrator_##CLASS_NAME()																									\
	{																																	\
		ComponentSystem::get()->addInitCallback(Unigine::MakeCallback(this, &ComponentRegistrator_##CLASS_NAME::register_component));	\
	}																																	\
	void register_component()																											\
	{																																	\
		ComponentSystem::get()->registerComponent<CLASS_NAME>();																		\
	}																																	\
} __component_registrator_##CLASS_NAME;

#ifndef __GNUC__
#pragma endregion Macros
#endif

////////////////////////////////////////////////////////////////////////////////////////
// Component Factory
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __GNUC__
#pragma region Factory
#endif

class ComponentCreatorInterface
{
public:
    virtual ~ComponentCreatorInterface() {}
    virtual ComponentBase* create ( const Unigine::NodePtr &node, int num ) const = 0;
};

template <class C>
class ComponentCreator : public ComponentCreatorInterface
{
public:
    virtual ComponentBase* create ( const Unigine::NodePtr &node, int num ) const
    {
        return new C ( node, num );
    }
};

class ComponentFactory
{
public:
    ComponentFactory() {}

    virtual ~ComponentFactory()
    {
        for ( auto i = map.begin(); i != map.end(); ++i )
            delete i->data;
    }

    template <class C>
    void add ( const char* name )
    {
        Unigine::String str ( name );
        auto o = map.find ( str );
        if ( o == map.end() )
            map.append ( str, new ComponentCreator<C>() );
    }

    ComponentBase* create ( const char* name, const Unigine::NodePtr &node, int num )
    {
        Unigine::String str ( name );
        auto o = map.find ( str );
        if ( o != map.end() )
            return o->data->create ( node, num );
        return nullptr;
    }

    void getComponentNames ( Unigine::Vector<Unigine::String> &names )
    {
        names = map.keys();
    }

private:
    Unigine::Map<Unigine::String, ComponentCreatorInterface*> map; // <property name, component creator>
};

#ifndef __GNUC__
#pragma endregion Factory
#endif

////////////////////////////////////////////////////////////////////////////////////////
// Component System
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __GNUC__
#pragma region System
#endif

class ComponentSystem : private Unigine::WorldLogic
{
public:
    // return pointer to singleton ComponentSystem
    static ComponentSystem *get()
    {
        static ComponentSystem instance;
        return &instance;
    }

    // initialize Component System and run all "init callbacks"
    void initialize();

    // adds a callback that is called during initialize()
    void addInitCallback ( Unigine::CallbackBase *callback );

    // register user object derived from ComponentBase class
    template <class C>
    void registerComponent()
    {
        component_factory.add<C> ( C::getPropertyName() );

        // create property file and load if it isn't exist
        Unigine::PropertyPtr prop = Unigine::Properties::findManualProperty ( C::getPropertyName() );
        if ( !prop )
            createPropertyFile ( C::getPropertyName() );
    }

    // create property files for all registered components in data folder ComponentSystem/
    void createPropertyFiles();

    // create property file for one registered component ("name" - name of the property)
    void createPropertyFile ( const char *name );

    template <class C>
    void createPropertyFile()
    {
        createPropertyFile ( C::getPropertyName() );
    }

    // rewrite property file and reload it in Property Manager
    void refreshProperty ( const char *name );

    template <class C>
    void refreshProperty()
    {
        refreshProperty ( C::getPropertyName() );
    }

    // nodes
    UNIGINE_INLINE void destroyNode ( const Unigine::NodePtr &node )
    {
        destroy_node_queue.append ( node );
    }

    // components
    template <class C>
    C *addComponent ( const Unigine::NodePtr &node )
    {
        if ( node.isDeleted() )
            return nullptr;

        int prev_count = components[node->getID()].size();
        node->addProperty ( C::getPropertyName() );
        if ( prev_count + 1 != components[node->getID()].size() ) // add check
            return nullptr;
        return dynamic_cast<C*> ( components[node->getID()][prev_count] );
    }

    template <class C>
    C *getComponent ( const Unigine::NodePtr &node )
    {
        if ( node.isDeleted() )
            return nullptr;

        auto it = components.find ( node->getID() );
        if ( it == components.end() )
            return nullptr;

        Unigine::String name = C::getPropertyName();
        int count = it->data.size();
        for ( int i = 0; i < count; i++ )
        {
            C *c = dynamic_cast<C*> ( it->data[i] );
            if ( c )
                return c;
        }
        return nullptr;
    }

    template <class C>
    C *getComponentInParent ( const Unigine::NodePtr &node )
    {
        Unigine::NodePtr n = node;
        C *c = nullptr;
        while ( !c && n )
        {
            c = getComponent<C> ( n );
            n = n->getParent() ? n->getParent() : n->getPossessor();
        }
        return c;
    }

    template <class C>
    C *getComponentInChildren ( const Unigine::NodePtr &node )
    {
        if ( node.isDeleted() )
            return nullptr;

        // find in current node
        C *c = getComponent<C> ( node );
        if ( c )
            return c;

        // find in NodeReference
        if ( node->getType() == Unigine::Node::NODE_REFERENCE )
        {
            Unigine::NodeReferencePtr node_ref = Unigine::static_ptr_cast<Unigine::NodeReference> ( node );
            c = getComponentInChildren<C> ( node_ref->getReference() );
            if ( c )
                return c;
        }

        // find in nearest children
        for ( int i = 0; i < node->getNumChildren(); i++ )
        {
            c = getComponent<C> ( node->getChild ( i ) );
            if ( c )
                return c;
        }

        // find in children of children
        for ( int i = 0; i < node->getNumChildren(); i++ )
        {
            c = getComponentInChildren<C> ( node->getChild ( i ) );
            if ( c )
                return c;
        }

        return nullptr;
    }

    template <class C>
    void getComponents ( const Unigine::NodePtr &node, Unigine::Vector<C*> &out_components, int clear_vector = 1 )
    {
        if ( clear_vector )
            out_components.clear();

        if ( node.isDeleted() )
            return;

        auto it = components.find ( node->getID() );
        if ( it == components.end() )
            return;

        int count = it->data.size();
        for ( int i = 0; i < count; i++ )
        {
            C *c = dynamic_cast<C*> ( it->data[i] );
            if ( c )
                out_components.append ( c );
        }
    }

    template <class C>
    void getComponentsInParent ( const Unigine::NodePtr &node, Unigine::Vector<C*> &out_components, int clear_vector = 1 )
    {
        if ( clear_vector )
            out_components.clear();

        Unigine::NodePtr n = node;
        while ( n )
        {
            getComponents<C> ( n, out_components, 0 );
            n = n->getParent() ? n->getParent() : n->getPossessor();
        }
    }

    template <class C>
    void getComponentsInChildren ( const Unigine::NodePtr &node, Unigine::Vector<C*> &out_components, int clear_vector = 1 )
    {
        if ( clear_vector )
            out_components.clear();

        if ( node.isDeleted() )
            return;

        // find in current node
        getComponents<C> ( node, out_components, 0 );

        // find in NodeReference
        if ( node->getType() == Unigine::Node::NODE_REFERENCE )
        {
            Unigine::NodeReferencePtr node_ref = Unigine::static_ptr_cast<Unigine::NodeReference> ( node );
            getComponentsInChildren<C> ( node_ref->getReference(), out_components, 0 );
        }

        // find in children
        for ( int i = 0; i < node->getNumChildren(); i++ )
            getComponentsInChildren<C> ( node->getChild ( i ), out_components, 0 );
    }

    template <class C>
    int removeComponent ( const Unigine::NodePtr &node )
    {
        if ( node.isNull() )
            return 0;

        auto it = components.find ( node->getID() );
        if ( it == components.end() )
            return 0;

        Unigine::String name = C::getPropertyName();
        int count = it->data.size();
        for ( int i = 0; i < count; i++ )
        {
            C *c = dynamic_cast<C*> ( it->data[i] );
            if ( c )
            {
                node->removeProperty ( i );
                return 1;
            }
        }
        return 0;
    }

    // statistics
    int getNumComponents();	// slow, use this method rarely
    UNIGINE_INLINE int getNumNodesWithComponents()
    {
        return components.size();
    }

    // warnings
    enum WARNING_LEVEL
    {
        NONE,
        LOW,	// when parameter types are incorrect only
        HIGH	// + when NODE/PROPERTY/MATERIAL parameters is equal to zero on startup
    };
    UNIGINE_INLINE void setWarningLevel ( WARNING_LEVEL level )
    {
        warning_level = level;
    }
    UNIGINE_INLINE int getWarningLevel()
    {
        return warning_level;
    }

    // method registators
#define METHOD_REGISTRATOR(METHOD, METHOD_LOWCASE)																	\
	void addComponentMethod##METHOD(ComponentBase *component,														\
		Unigine::CallbackBase *func, const char *name = nullptr, const char *func_name = nullptr, int order = 0, bool invoke_disabled = false);	\
	void removeComponentMethod##METHOD(ComponentBase *component,													\
		 Unigine::CallbackBase *func, const char *name = nullptr, const char *func_name = nullptr, int order = 0, bool invoke_disabled = false);
    METHOD_REGISTRATOR ( Init, init );
    METHOD_REGISTRATOR ( UpdateAsyncThread, updateAsyncThread );
    METHOD_REGISTRATOR ( UpdateSyncThread, updateSyncThread );
    METHOD_REGISTRATOR ( Update, update );
    METHOD_REGISTRATOR ( PostUpdate, postUpdate );
    METHOD_REGISTRATOR ( UpdatePhysics, updatePhysics );
    METHOD_REGISTRATOR ( Swap, swap );
    METHOD_REGISTRATOR ( Shutdown, shutdown );
    METHOD_REGISTRATOR ( DestroyRenderResources, destroyRenderResources );
#undef METHOD_REGISTRATOR

private:
    // world's main loop
    int init() override;
    void updateAsyncThread ( int id, int size ) override;
    void updateSyncThread ( int id, int size ) override;
    int update() override;
    int postUpdate() override;
    int updatePhysics() override;
    int swap() override;
    int shutdown() override;
    int destroyRenderResources() override;

private:
    ComponentSystem();
    virtual ~ComponentSystem();
    ComponentSystem ( ComponentSystem const& ) = delete;
    ComponentSystem& operator= ( ComponentSystem const& ) = delete;

    // call component methods
    struct ComponentCallback;
    void run_init_methods();
    void run_thread_methods ( const Unigine::Map<int, Unigine::Vector<ComponentCallback>> &functions, int id, int size );

    // callbacks
    void on_property_slots_changed ( const Unigine::NodePtr node, int num_slots );
    void on_property_created ( const Unigine::NodePtr node, const Unigine::PropertyPtr prop, int num );
    void on_property_swapped ( const Unigine::NodePtr node, int from_num, int to_num );
    void on_property_removed ( const Unigine::NodePtr node, const Unigine::PropertyPtr prop, int num );
    void on_property_change_enabled ( const Unigine::NodePtr node, const Unigine::PropertyPtr prop, int num );
    void on_node_change_enabled ( const Unigine::NodePtr node );

    struct QueuedCallback
    {
        enum TYPE { SLOTS_CHANGED, CREATED, SWAPPED, REMOVED, CHANGE_ENABLED, NODE_CHANGE_ENABLED };
        TYPE type;
        int node;
        Unigine::UGUID prop;
        int from_num;
        int num;
    };
    Unigine::Vector<QueuedCallback> queued_callbacks;
    mutable Unigine::Mutex callbacks_lock;

    // initialization
    Unigine::Vector<Unigine::CallbackBase *> initialize_callbacks;
    int initialized;

    // world state
    int shutdowned;

    // nodes to destroy
    void destroy_nodes();
    void destroy_node_with_children ( Unigine::NodePtr &node );
    Unigine::Vector<Unigine::NodePtr> destroy_node_queue;

    // profiler
    long long profiler_time;
    void render_profiler();

    // warnings
    int warning_level;

    // components with properties
    ComponentFactory component_factory;
    Unigine::Map<int, Unigine::Vector<ComponentBase*>> components; // <node ID, components[]>

    struct ComponentCallback
    {
        ComponentBase *component;
        Unigine::CallbackBase *function;
        const char *name; // example: "ClassName::functionName", used by MicroProfiler
        const char *func_name; // example: "functionName", used to find overrides
        bool called;
        bool invoke_disabled;

        bool operator== ( const ComponentCallback &other ) const
        {
            return component == other.component && function == other.function;
        }
    };

#define VECTOR_OF_COMPONENT_FUNCTIONS(NAME) \
	Unigine::Map<int, Unigine::Vector<ComponentCallback>> component_functions_##NAME; // <order, component's functions in creation order>
    VECTOR_OF_COMPONENT_FUNCTIONS ( init );
    VECTOR_OF_COMPONENT_FUNCTIONS ( updateAsyncThread );
    VECTOR_OF_COMPONENT_FUNCTIONS ( updateSyncThread );
    VECTOR_OF_COMPONENT_FUNCTIONS ( update );
    VECTOR_OF_COMPONENT_FUNCTIONS ( postUpdate );
    VECTOR_OF_COMPONENT_FUNCTIONS ( updatePhysics );
    VECTOR_OF_COMPONENT_FUNCTIONS ( swap );
    VECTOR_OF_COMPONENT_FUNCTIONS ( shutdown );
    VECTOR_OF_COMPONENT_FUNCTIONS ( destroyRenderResources );
#undef VECTOR_OF_COMPONENT_FUNCTIONS
    Unigine::Map<int, Unigine::Vector<ComponentCallback>> component_functions_init_delayed;
};

#ifndef __GNUC__
#pragma endregion System
#endif

////////////////////////////////////////////////////////////////////////////////////////
// Component Variables
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __GNUC__
#pragma region Variables
#endif

class ComponentVariable;
class ComponentStruct;

// class that stores info about property, its parameters and structs
class PropertyWrapper
{
public:
    PropertyWrapper() = default;
    PropertyWrapper ( const Unigine::PropertyPtr &prop );
    virtual ~PropertyWrapper() = default;

    // pointer to the property
    Unigine::PropertyPtr property;

    // parameter wrappers
    Unigine::Vector<ComponentVariable*> variables;

    // struct wrappers
    struct ComponentStructInfo
    {
        Unigine::String name;
        ComponentStruct *instance;
        ComponentVariable *variable;
    };
    Unigine::Vector<ComponentStructInfo> structs;

    // helper methods
    int findStructIndex ( const char *name );
};

class ComponentVariable
{
public:
    ComponentVariable() = default;
    ComponentVariable ( PropertyWrapper *component, const char *name, int type,
                        const char *title, const char *tooltip, const char *group );
    virtual ~ComponentVariable();

    UNIGINE_INLINE int getID() const
    {
        return parameter->getID();
    }
    UNIGINE_INLINE const char* getName() const
    {
        return name.get();
    }
    UNIGINE_INLINE int getType() const
    {
        return type;
    }
    const char *getTypeName() const;
    UNIGINE_INLINE virtual Unigine::String getValueAsString()
    {
        return Unigine::String::null;
    }
    UNIGINE_INLINE virtual int nullCheck()
    {
        return 0;
    }

    virtual void save ( const Unigine::XmlPtr &parameter );

    virtual void setParameter ( const Unigine::PropertyParameterPtr &param );

protected:
    int is_type_name ( const char *name ) const;

    PropertyWrapper *holder;

    Unigine::PropertyParameterPtr parameter;
    Unigine::String name;
    Unigine::String title;
    Unigine::String tooltip;
    Unigine::String group;
    int type;
};

class ComponentVariableInt : public ComponentVariable
{
public:
    ComponentVariableInt() = default;
    ComponentVariableInt ( PropertyWrapper* component, const char *name, int default_value = 0,
                           const char *title = nullptr, const char *tooltip = nullptr, const char *group = nullptr )
        : ComponentVariable ( component, name, Unigine::Property::PARAMETER_INT, title, tooltip, group ), value ( default_value ) {}
    virtual ~ComponentVariableInt() = default;

    ComponentVariableInt &operator= ( int value );
    operator int() const;
    int get() const;
    UNIGINE_INLINE Unigine::String getValueAsString() override
    {
        return Unigine::String::itoa ( value );
    }

protected:
    int value;
};

class ComponentVariableToggle : public ComponentVariable
{
public:
    ComponentVariableToggle() = default;
    ComponentVariableToggle ( PropertyWrapper* component, const char *name, int default_value = 0,
                              const char *title = nullptr, const char *tooltip = nullptr, const char *group = nullptr )
        : ComponentVariable ( component, name, Unigine::Property::PARAMETER_TOGGLE, title, tooltip, group ), value ( default_value ) {}
    virtual ~ComponentVariableToggle() = default;

    ComponentVariableToggle &operator= ( int value );
    operator int() const;
    int get() const;
    UNIGINE_INLINE Unigine::String getValueAsString() override
    {
        return Unigine::String::itoa ( value );
    }

protected:
    int value;
};

class ComponentVariableSwitch : public ComponentVariable
{
public:
    ComponentVariableSwitch() = default;
    ComponentVariableSwitch ( PropertyWrapper* component, const char *name, int default_value = 0, const char *in_items = nullptr,
                              const char *title = nullptr, const char *tooltip = nullptr, const char *group = nullptr )
        : ComponentVariable ( component, name, Unigine::Property::PARAMETER_SWITCH, title, tooltip, group ), value ( default_value ), items ( in_items ) {}
    virtual ~ComponentVariableSwitch() = default;

    ComponentVariableSwitch &operator= ( int value );
    operator int() const;
    int get() const;
    UNIGINE_INLINE Unigine::String getValueAsString() override
    {
        return Unigine::String::itoa ( value );
    }

    void save ( const Unigine::XmlPtr &parameter ) override;

protected:
    int value;
    Unigine::String items;
};

class ComponentVariableMask : public ComponentVariable
{
public:
    ComponentVariableMask() = default;
    ComponentVariableMask ( PropertyWrapper* component, const char *name, int default_value = 0,
                            const char *title = nullptr, const char *tooltip = nullptr, const char *group = nullptr )
        : ComponentVariable ( component, name, Unigine::Property::PARAMETER_MASK, title, tooltip, group ), value ( default_value ) {}
    ComponentVariableMask ( PropertyWrapper* component, const char *name, const char *mask_type, int default_value = 0,
                            const char *title = nullptr, const char *tooltip = nullptr, const char *group = nullptr )
        : ComponentVariable ( component, name, Unigine::Property::PARAMETER_MASK, title, tooltip, group ), value ( default_value ), flags ( mask_type ) {}
    virtual ~ComponentVariableMask() = default;

    ComponentVariableMask &operator= ( int value );
    operator int() const;
    int get() const;
    UNIGINE_INLINE Unigine::String getValueAsString() override
    {
        return Unigine::String::itoa ( value );
    }

    void save ( const Unigine::XmlPtr &parameter ) override;

protected:
    int value;
    Unigine::String flags;
};

class ComponentVariableFloat : public ComponentVariable
{
public:
    ComponentVariableFloat() = default;
    ComponentVariableFloat ( PropertyWrapper* component, const char *name, float default_value = 0,
                             const char *title = nullptr, const char *tooltip = nullptr, const char *group = nullptr )
        : ComponentVariable ( component, name, Unigine::Property::PARAMETER_FLOAT, title, tooltip, group ), value ( default_value ) {}
    virtual ~ComponentVariableFloat() = default;

    ComponentVariableFloat &operator= ( float value );
    operator float() const;
    float get() const;
    UNIGINE_INLINE Unigine::String getValueAsString() override
    {
        return Unigine::String::ftoa ( value );
    }

protected:
    float value;
};

class ComponentVariableDouble : public ComponentVariable
{
public:
    ComponentVariableDouble() = default;
    ComponentVariableDouble ( PropertyWrapper* component, const char *name, double default_value = 0,
                              const char *title = nullptr, const char *tooltip = nullptr, const char *group = nullptr )
        : ComponentVariable ( component, name, Unigine::Property::PARAMETER_DOUBLE, title, tooltip, group ), value ( default_value ) {}
    virtual ~ComponentVariableDouble() = default;

    ComponentVariableDouble &operator= ( double value );
    operator double() const;
    double get() const;
    UNIGINE_INLINE Unigine::String getValueAsString() override
    {
        return Unigine::String::dtoa ( value );
    }

protected:
    double value;
};

class ComponentVariableString : public ComponentVariable
{
public:
    ComponentVariableString() = default;
    ComponentVariableString ( PropertyWrapper* component, const char *name, const char *default_value = nullptr,
                              const char *title = nullptr, const char *tooltip = nullptr, const char *group = nullptr )
        : ComponentVariable ( component, name, Unigine::Property::PARAMETER_STRING, title, tooltip, group ), value ( default_value ) {}
    virtual ~ComponentVariableString() = default;

    ComponentVariableString &operator= ( const char *value );
    operator const char *() const;
    const char *get() const;
    UNIGINE_INLINE Unigine::String getValueAsString() override
    {
        return value;
    }

protected:
    mutable Unigine::String value;
};

class ComponentVariableVec2 : public ComponentVariable
{
public:
    ComponentVariableVec2() = default;
    ComponentVariableVec2 ( PropertyWrapper* component, const char *name,
                            const char *title = nullptr, const char *tooltip = nullptr, const char *group = nullptr )
        : ComponentVariable ( component, name, Unigine::Property::PARAMETER_VEC2, title, tooltip, group ) {}
    ComponentVariableVec2 ( PropertyWrapper* component, const char *name, const Unigine::Math::vec2 &default_value,
                            const char *title = nullptr, const char *tooltip = nullptr, const char *group = nullptr )
        : ComponentVariable ( component, name, Unigine::Property::PARAMETER_VEC2, title, tooltip, group ), value ( default_value ) {}
    virtual ~ComponentVariableVec2() = default;

    ComponentVariableVec2 &operator= ( const Unigine::Math::vec2 &value );
    operator Unigine::Math::vec2() const;
    Unigine::Math::vec2 get() const;
    UNIGINE_INLINE Unigine::String getValueAsString() override
    {
        return Unigine::String::format ( "%f %f", value.x, value.y );
    }

protected:
    Unigine::Math::vec2 value;
};

class ComponentVariableVec3 : public ComponentVariable
{
public:
    ComponentVariableVec3() = default;
    ComponentVariableVec3 ( PropertyWrapper* component, const char *name,
                            const char *title = nullptr, const char *tooltip = nullptr, const char *group = nullptr )
        : ComponentVariable ( component, name, Unigine::Property::PARAMETER_VEC3, title, tooltip, group ) {}
    ComponentVariableVec3 ( PropertyWrapper* component, const char *name, const Unigine::Math::vec3 &default_value,
                            const char *title = nullptr, const char *tooltip = nullptr, const char *group = nullptr )
        : ComponentVariable ( component, name, Unigine::Property::PARAMETER_VEC3, title, tooltip, group ), value ( default_value ) {}
    virtual ~ComponentVariableVec3() = default;

    ComponentVariableVec3 &operator= ( const Unigine::Math::vec3 &value );
    operator Unigine::Math::vec3() const;
    Unigine::Math::vec3 get() const;
    UNIGINE_INLINE Unigine::String getValueAsString() override
    {
        return Unigine::String::format ( "%f %f %f", value.x, value.y, value.z );
    }

protected:
    Unigine::Math::vec3 value;
};

class ComponentVariableVec4 : public ComponentVariable
{
public:
    ComponentVariableVec4() = default;
    ComponentVariableVec4 ( PropertyWrapper* component, const char *name,
                            const char *title = nullptr, const char *tooltip = nullptr, const char *group = nullptr )
        : ComponentVariable ( component, name, Unigine::Property::PARAMETER_VEC4, title, tooltip, group ) {}
    ComponentVariableVec4 ( PropertyWrapper* component, const char *name, const Unigine::Math::vec4 &default_value,
                            const char *title = nullptr, const char *tooltip = nullptr, const char *group = nullptr )
        : ComponentVariable ( component, name, Unigine::Property::PARAMETER_VEC4, title, tooltip, group ), value ( default_value ) {}
    virtual ~ComponentVariableVec4() = default;

    ComponentVariableVec4 &operator= ( const Unigine::Math::vec4 &value );
    operator Unigine::Math::vec4() const;
    Unigine::Math::vec4 get() const;
    UNIGINE_INLINE Unigine::String getValueAsString() override
    {
        return Unigine::String::format ( "%f %f %f %f", value.x, value.y, value.z, value.w );
    }

protected:
    Unigine::Math::vec4 value;
};

class ComponentVariableDVec2 : public ComponentVariable
{
public:
    ComponentVariableDVec2() = default;
    ComponentVariableDVec2 ( PropertyWrapper* component, const char *name,
                             const char *title = nullptr, const char *tooltip = nullptr, const char *group = nullptr )
        : ComponentVariable ( component, name, Unigine::Property::PARAMETER_DVEC2, title, tooltip, group ) {}
    ComponentVariableDVec2 ( PropertyWrapper* component, const char *name, const Unigine::Math::dvec2 &default_value,
                             const char *title = nullptr, const char *tooltip = nullptr, const char *group = nullptr )
        : ComponentVariable ( component, name, Unigine::Property::PARAMETER_DVEC2, title, tooltip, group ), value ( default_value ) {}
    virtual ~ComponentVariableDVec2() = default;

    ComponentVariableDVec2 &operator= ( const Unigine::Math::dvec2 &value );
    operator Unigine::Math::dvec2() const;
    Unigine::Math::dvec2 get() const;
    UNIGINE_INLINE Unigine::String getValueAsString() override
    {
        return Unigine::String::format ( "%lf %lf", value.x, value.y );
    }

protected:
    Unigine::Math::dvec2 value;
};

class ComponentVariableDVec3 : public ComponentVariable
{
public:
    ComponentVariableDVec3() = default;
    ComponentVariableDVec3 ( PropertyWrapper* component, const char *name,
                             const char *title = nullptr, const char *tooltip = nullptr, const char *group = nullptr )
        : ComponentVariable ( component, name, Unigine::Property::PARAMETER_DVEC3, title, tooltip, group ) {}
    ComponentVariableDVec3 ( PropertyWrapper* component, const char *name, const Unigine::Math::dvec3 &default_value,
                             const char *title = nullptr, const char *tooltip = nullptr, const char *group = nullptr )
        : ComponentVariable ( component, name, Unigine::Property::PARAMETER_DVEC3, title, tooltip, group ), value ( default_value ) {}
    virtual ~ComponentVariableDVec3() = default;

    ComponentVariableDVec3 &operator= ( const Unigine::Math::dvec3 &value );
    operator Unigine::Math::dvec3() const;
    Unigine::Math::dvec3 get() const;
    UNIGINE_INLINE Unigine::String getValueAsString() override
    {
        return Unigine::String::format ( "%lf %lf %lf", value.x, value.y, value.z );
    }

protected:
    Unigine::Math::dvec3 value;
};

class ComponentVariableDVec4 : public ComponentVariable
{
public:
    ComponentVariableDVec4() = default;
    ComponentVariableDVec4 ( PropertyWrapper* component, const char *name,
                             const char *title = nullptr, const char *tooltip = nullptr, const char *group = nullptr )
        : ComponentVariable ( component, name, Unigine::Property::PARAMETER_DVEC4, title, tooltip, group ) {}
    ComponentVariableDVec4 ( PropertyWrapper* component, const char *name, const Unigine::Math::dvec4 &default_value,
                             const char *title = nullptr, const char *tooltip = nullptr, const char *group = nullptr )
        : ComponentVariable ( component, name, Unigine::Property::PARAMETER_DVEC4, title, tooltip, group ), value ( default_value ) {}
    virtual ~ComponentVariableDVec4() = default;

    ComponentVariableDVec4 &operator= ( const Unigine::Math::dvec4 &value );
    operator Unigine::Math::dvec4() const;
    Unigine::Math::dvec4 get() const;
    UNIGINE_INLINE Unigine::String getValueAsString() override
    {
        return Unigine::String::format ( "%lf %lf %lf %lf", value.x, value.y, value.z, value.w );
    }

protected:
    Unigine::Math::dvec4 value;
};

class ComponentVariableIVec2 : public ComponentVariable
{
public:
    ComponentVariableIVec2() = default;
    ComponentVariableIVec2 ( PropertyWrapper* component, const char *name,
                             const char *title = nullptr, const char *tooltip = nullptr, const char *group = nullptr )
        : ComponentVariable ( component, name, Unigine::Property::PARAMETER_IVEC2, title, tooltip, group ) {}
    ComponentVariableIVec2 ( PropertyWrapper* component, const char *name, const Unigine::Math::ivec2 &default_value,
                             const char *title = nullptr, const char *tooltip = nullptr, const char *group = nullptr )
        : ComponentVariable ( component, name, Unigine::Property::PARAMETER_IVEC2, title, tooltip, group ), value ( default_value ) {}
    virtual ~ComponentVariableIVec2() = default;

    ComponentVariableIVec2 &operator= ( const Unigine::Math::ivec2 &value );
    operator Unigine::Math::ivec2() const;
    Unigine::Math::ivec2 get() const;
    UNIGINE_INLINE Unigine::String getValueAsString() override
    {
        return Unigine::String::format ( "%d %d", value.x, value.y );
    }

protected:
    Unigine::Math::ivec2 value;
};

class ComponentVariableIVec3 : public ComponentVariable
{
public:
    ComponentVariableIVec3() = default;
    ComponentVariableIVec3 ( PropertyWrapper* component, const char *name,
                             const char *title = nullptr, const char *tooltip = nullptr, const char *group = nullptr )
        : ComponentVariable ( component, name, Unigine::Property::PARAMETER_IVEC3, title, tooltip, group ) {}
    ComponentVariableIVec3 ( PropertyWrapper* component, const char *name, const Unigine::Math::ivec3 &default_value,
                             const char *title = nullptr, const char *tooltip = nullptr, const char *group = nullptr )
        : ComponentVariable ( component, name, Unigine::Property::PARAMETER_IVEC3, title, tooltip, group ), value ( default_value ) {}
    virtual ~ComponentVariableIVec3() = default;

    ComponentVariableIVec3 &operator= ( const Unigine::Math::ivec3 &value );
    operator Unigine::Math::ivec3() const;
    Unigine::Math::ivec3 get() const;
    UNIGINE_INLINE Unigine::String getValueAsString() override
    {
        return Unigine::String::format ( "%d %d %d", value.x, value.y, value.z );
    }

protected:
    Unigine::Math::ivec3 value;
};

class ComponentVariableIVec4 : public ComponentVariable
{
public:
    ComponentVariableIVec4() = default;
    ComponentVariableIVec4 ( PropertyWrapper* component, const char *name,
                             const char *title = nullptr, const char *tooltip = nullptr, const char *group = nullptr )
        : ComponentVariable ( component, name, Unigine::Property::PARAMETER_IVEC4, title, tooltip, group ) {}
    ComponentVariableIVec4 ( PropertyWrapper* component, const char *name, const Unigine::Math::ivec4 &default_value,
                             const char *title = nullptr, const char *tooltip = nullptr, const char *group = nullptr )
        : ComponentVariable ( component, name, Unigine::Property::PARAMETER_IVEC4, title, tooltip, group ), value ( default_value ) {}
    virtual ~ComponentVariableIVec4() = default;

    ComponentVariableIVec4 &operator= ( const Unigine::Math::ivec4 &value );
    operator Unigine::Math::ivec4() const;
    Unigine::Math::ivec4 get() const;
    UNIGINE_INLINE Unigine::String getValueAsString() override
    {
        return Unigine::String::format ( "%d %d %d %d", value.x, value.y, value.z, value.w );
    }

protected:
    Unigine::Math::ivec4 value;
};

class ComponentVariableColor : public ComponentVariable
{
public:
    ComponentVariableColor() = default;
    ComponentVariableColor ( PropertyWrapper* component, const char *name,
                             const char *title = nullptr, const char *tooltip = nullptr, const char *group = nullptr )
        : ComponentVariable ( component, name, Unigine::Property::PARAMETER_COLOR, title, tooltip, group ) {}
    ComponentVariableColor ( PropertyWrapper* component, const char *name, const Unigine::Math::vec4 &default_value,
                             const char *title = nullptr, const char *tooltip = nullptr, const char *group = nullptr )
        : ComponentVariable ( component, name, Unigine::Property::PARAMETER_COLOR, title, tooltip, group ), value ( default_value ) {}
    virtual ~ComponentVariableColor() = default;

    ComponentVariableColor &operator= ( const Unigine::Math::vec4 &value );
    operator Unigine::Math::vec4() const;
    Unigine::Math::vec4 get() const;
    UNIGINE_INLINE Unigine::String getValueAsString() override
    {
        return Unigine::String::format ( "%f %f %f %f", value.x, value.y, value.z, value.w );
    }

protected:
    Unigine::Math::vec4 value;
};

class ComponentVariableFile : public ComponentVariable
{
public:
    ComponentVariableFile() = default;
    ComponentVariableFile ( PropertyWrapper* component, const char *name, const char *default_value = nullptr,
                            const char *title = nullptr, const char *tooltip = nullptr, const char *group = nullptr )
        : ComponentVariable ( component, name, Unigine::Property::PARAMETER_FILE, title, tooltip, group ), value ( default_value ) {}
    virtual ~ComponentVariableFile() = default;

    ComponentVariableFile &operator= ( const char *value );
    operator const char*() const;
    const char *get() const;
    const char *getRaw() const;
    UNIGINE_INLINE Unigine::String getValueAsString() override
    {
        return value;
    }
    UNIGINE_INLINE int nullCheck() override
    {
        return parameter ? ( !parameter->isFileExist() ) : 0;
    }

protected:
    mutable Unigine::String value;
};

class ComponentVariableProperty : public ComponentVariable
{
public:
    ComponentVariableProperty() = default;
    ComponentVariableProperty ( PropertyWrapper* component, const char *name,
                                const char *title = nullptr, const char *tooltip = nullptr, const char *group = nullptr )
        : ComponentVariable ( component, name, Unigine::Property::PARAMETER_PROPERTY, title, tooltip, group ) {}
    ComponentVariableProperty ( PropertyWrapper* component, const char *name, const Unigine::PropertyPtr &default_value,
                                const char *title = nullptr, const char *tooltip = nullptr, const char *group = nullptr )
        : ComponentVariable ( component, name, Unigine::Property::PARAMETER_PROPERTY, title, tooltip, group )
        , value ( default_value )
        , value_guid ( default_value->getGUID() ) {}

    virtual ~ComponentVariableProperty() = default;

    ComponentVariableProperty &operator= ( const Unigine::PropertyPtr &value );
    ComponentVariableProperty &operator= ( const Unigine::UGUID &value );
    operator Unigine::PropertyPtr() const;
    operator Unigine::UGUID() const;
    Unigine::PropertyPtr get() const;
    Unigine::Property *operator->();
    UNIGINE_INLINE int isEmpty() const
    {
        return get().get() == nullptr;
    }
    UNIGINE_INLINE Unigine::String getValueAsString() override
    {
        return value_guid.getString();
    }
    UNIGINE_INLINE int nullCheck() override
    {
        return isEmpty();
    }

protected:
    Unigine::PropertyPtr value;
    Unigine::UGUID value_guid;
};

class ComponentVariableMaterial : public ComponentVariable
{
public:
    ComponentVariableMaterial() = default;
    ComponentVariableMaterial ( PropertyWrapper* component, const char *name,
                                const char *title = nullptr, const char *tooltip = nullptr, const char *group = nullptr )
        : ComponentVariable ( component, name, Unigine::Property::PARAMETER_MATERIAL, title, tooltip, group ) {}
    ComponentVariableMaterial ( PropertyWrapper* component, const char *name, const Unigine::MaterialPtr &default_value,
                                const char *title = nullptr, const char *tooltip = nullptr, const char *group = nullptr )
        : ComponentVariable ( component, name, Unigine::Property::PARAMETER_MATERIAL, title, tooltip, group )
        , value ( default_value )
        , value_guid ( default_value->getGUID() ) {}

    virtual ~ComponentVariableMaterial() = default;

    ComponentVariableMaterial &operator= ( const Unigine::MaterialPtr &value );
    ComponentVariableMaterial &operator= ( const Unigine::UGUID &value );
    operator Unigine::MaterialPtr() const;
    operator Unigine::UGUID() const;
    Unigine::MaterialPtr get() const;
    Unigine::Material *operator->();
    UNIGINE_INLINE int isEmpty() const
    {
        return get().get() == nullptr;
    }
    UNIGINE_INLINE Unigine::String getValueAsString() override
    {
        return value_guid.getString();
    }
    UNIGINE_INLINE int nullCheck() override
    {
        return isEmpty();
    }

protected:
    Unigine::MaterialPtr value;
    Unigine::UGUID value_guid;
};

class ComponentVariableNode : public ComponentVariable
{
public:
    ComponentVariableNode() = default;
    ComponentVariableNode ( PropertyWrapper* component, const char *name,
                            const char *title = nullptr, const char *tooltip = nullptr, const char *group = nullptr )
        : ComponentVariable ( component, name, Unigine::Property::PARAMETER_NODE, title, tooltip, group )
        , value_id ( 0 ) {}
    ComponentVariableNode ( PropertyWrapper* component, const char *name, const Unigine::NodePtr &default_value,
                            const char *title = nullptr, const char *tooltip = nullptr, const char *group = nullptr )
        : ComponentVariable ( component, name, Unigine::Property::PARAMETER_NODE, title, tooltip, group )
        , value ( default_value )
        , value_id ( default_value->getID() ) {}

    virtual ~ComponentVariableNode() = default;

    ComponentVariableNode &operator= ( const Unigine::NodePtr &value );
    ComponentVariableNode &operator= ( int value );
    operator Unigine::NodePtr() const;
    operator int() const;
    Unigine::NodePtr get() const;
    Unigine::Node *operator->();
    UNIGINE_INLINE int isEmpty() const
    {
        return get().get() == nullptr;
    }
    UNIGINE_INLINE Unigine::String getValueAsString() override
    {
        return Unigine::String::itoa ( value_id );
    }
    UNIGINE_INLINE int nullCheck() override
    {
        return isEmpty();
    }

protected:
    Unigine::NodePtr value;
    int value_id;
};

class ComponentVariableStructBase : public ComponentVariable
{
public:
    ComponentVariableStructBase() = default;
    ComponentVariableStructBase ( PropertyWrapper *component, const char *type_name,
                                  const char *name = nullptr, const char *title = nullptr, const char *tooltip = nullptr, const char *group = nullptr )
        : ComponentVariable ( component, name, Unigine::Property::PARAMETER_STRUCT, title, tooltip, group ) {}
    virtual ~ComponentVariableStructBase() = default;

    UNIGINE_INLINE ComponentStruct *getBase() const
    {
        return value_base;
    }
    UNIGINE_INLINE const Unigine::String &getStructName() const
    {
        return struct_type;
    }

    void save ( const Unigine::XmlPtr &parameter ) override;
    void setParameter ( const Unigine::PropertyParameterPtr &param ) override;

protected:
    Unigine::String struct_type;
    ComponentStruct *value_base;
};

template <class C>
class ComponentVariableStruct : public ComponentVariableStructBase
{
public:
    ComponentVariableStruct() = default;
    ComponentVariableStruct ( PropertyWrapper *component, const char *type_name,
                              const char *name = nullptr, const char *title = nullptr, const char *tooltip = nullptr, const char *group = nullptr )
        : ComponentVariableStructBase ( component, type_name, name, title, tooltip, group )
    {
        // set base variables
        value_base = &value;
        struct_type = type_name;

        // set parameters to all children recursively
        setParameter ( parameter );

        // add new struct to the Component
        int index = component->findStructIndex ( struct_type.get() );
        if ( index == -1 )
            component->structs.append ( { struct_type /*name*/, value_base, nullptr } );
    }
    virtual ~ComponentVariableStruct() = default;

    UNIGINE_INLINE C *operator->()
    {
        return &value;
    }
    UNIGINE_INLINE operator C() const
    {
        return value;
    }
    UNIGINE_INLINE C &get()
    {
        return value;
    }

protected:
    C value;
};

template <class C>
class ComponentVariableArray : public ComponentVariable
{
public:
    ComponentVariableArray() : struct_reference ( nullptr ) {}
    ComponentVariableArray ( PropertyWrapper *component, const char *name, const char *type_name,
                             const char *title = nullptr, const char *tooltip = nullptr, const char *group = nullptr )
        : ComponentVariable ( component, name, Unigine::Property::PARAMETER_ARRAY, title, tooltip, group )
        , struct_reference ( nullptr )
    {
        is_basic_type = is_type_name ( type_name );

        // all basic types ("int", "float") have lower characters instead of structs ("MyStruct")
        // so, we modify the name if we got "Int" or "Float" string in type_name
        value_type = type_name;
        if ( is_basic_type )
            value_type.lower();

        // add new struct to the Component
        if ( !is_basic_type )
        {
            int index = component->findStructIndex ( value_type.get() );
            if ( index == -1 )
            {
                struct_reference = new C { component, type_name };
                component->variables.removeLast(); // remove reference from component (it's added in ComponentVariable constructor)
                ComponentVariableStructBase *struct_reference_base = dynamic_cast<ComponentVariableStructBase*> ( struct_reference );
                component->structs.append ( { value_type /*name*/, struct_reference_base->getBase(), struct_reference } );
            }
        }
    }

    virtual ~ComponentVariableArray()
    {
        delete struct_reference;
    }

    UNIGINE_INLINE void resize ( int size )
    {
        int prev_size = value.size();

        // destroy old values
        for ( int i = size; i < prev_size; i++ )
            delete value[i];

        // resize
        value.resize ( size );
        if ( parameter )
            parameter->setArraySize ( size );

        // initialize new values
        for ( int i = prev_size; i < size; i++ )
        {
            if ( is_basic_type )
                value[i] = new C { holder, "" };
            else
                value[i] = new C { holder, value_type.get() };
        }

        // set actual PropertyParameter to children
        if ( parameter )
            for ( int i = 0; i < size; i++ )
                value[i]->setParameter ( parameter->getChild ( i ) );
    }

    UNIGINE_INLINE int size()
    {
        refresh();
        return ( int ) value.size();
    }
    UNIGINE_INLINE C &get ( int index )
    {
        refresh();
        return *value[index];
    }
    UNIGINE_INLINE C &operator[] ( int index )
    {
        refresh();
        return *value[index];
    }

    UNIGINE_INLINE void save ( const Unigine::XmlPtr &xml ) override
    {
        ComponentVariable::save ( xml );
        xml->setArg ( "array_type", value_type.get() );
    }

protected:
    Unigine::String value_type;
    int is_basic_type;
    Unigine::Vector<C*> value;
    C *struct_reference;

private:
    // refresh links from Property to Vector<C> value variable
    void refresh()
    {
        if ( parameter && parameter->getArraySize() != value.size() )
            resize ( parameter->getArraySize() );
    }
};

#ifndef __GNUC__
#pragma endregion Variables
#endif

////////////////////////////////////////////////////////////////////////////////////////
// Component Bases
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __GNUC__
#pragma region Base
#endif

// base class for the components
class ComponentBase: public PropertyWrapper
{
public:
    ComponentBase ( const Unigine::NodePtr &node, int num );
    virtual ~ComponentBase();

    UNIGINE_INLINE virtual const char *getClassName()
    {
        return "ComponentBase";
    }

    // property
    UNIGINE_INLINE static const char *getPropertyName()
    {
        return "component_base";
    }
    UNIGINE_INLINE virtual const char *getParentPropertyName()
    {
        return "node_base";
    }
    UNIGINE_INLINE virtual int isAutoSaveProperty()
    {
        return 1;
    }

    // common functions
    UNIGINE_INLINE const Unigine::NodePtr &getNode() const
    {
        return node;
    }
    UNIGINE_INLINE int getPropertyNum() const
    {
        return property_num;
    }
    UNIGINE_INLINE const Unigine::PropertyPtr &getProperty() const
    {
        return property;
    }

    UNIGINE_INLINE void setEnabled ( bool enable )
    {
        return node->setPropertyEnabled ( property_num, enable );
    }
    UNIGINE_INLINE int isEnabled() const
    {
        return enabled;
    }

    UNIGINE_INLINE int isInitialized() const
    {
        return all_init_called;
    }

    // callbacks
    UNIGINE_INLINE void setDestroyCallback ( Unigine::CallbackBase *func )
    {
        delete destroy_callback;
        destroy_callback = func;
    }
    UNIGINE_INLINE void clearDestroyCallback()
    {
        delete destroy_callback;
        destroy_callback = nullptr;
    }

protected:
    friend class ComponentSystem;
    friend class ComponentVariable;

    //////////////////////////////////////////////////////////
    // virtual
    //////////////////////////////////////////////////////////

    // callbacks
    virtual void on_enable() {}		// call once when property/node was enabled
    virtual void on_disable() {}	// ... was disabled
    virtual void on_ready() {}		// call once when component was created

    //////////////////////////////////////////////////////////
    // common
    //////////////////////////////////////////////////////////

    // check XMLs equals
    int is_equals ( const Unigine::XmlPtr &xml1, const Unigine::XmlPtr &xml2 );

    // saves all variables to .prop file
    void save_property ( const char *name );

    // nodes
    void destroyNode ( const Unigine::NodePtr &node );

    // components
    template <class C>
    C *addComponent ( const Unigine::NodePtr &node )
    {
        return ComponentSystem::get()->addComponent<C> ( node );
    }

    template <class C>
    C *getComponent ( const Unigine::NodePtr &node )
    {
        return ComponentSystem::get()->getComponent<C> ( node );
    }

    template <class C>
    C *getComponentInParent ( const Unigine::NodePtr &node )
    {
        return ComponentSystem::get()->getComponentInParent<C> ( node );
    }

    template <class C>
    C *getComponentInChildren ( const Unigine::NodePtr &node )
    {
        return ComponentSystem::get()->getComponentInChildren<C> ( node );
    }

    template <class C>
    void getComponents ( const Unigine::NodePtr &node, Unigine::Vector<C*> &components )
    {
        ComponentSystem::get()->getComponents<C> ( node, components );
    }

    template <class C>
    void getComponentsInParent ( const Unigine::NodePtr &node, Unigine::Vector<C*> &components )
    {
        ComponentSystem::get()->getComponentsInParent<C> ( node, components );
    }

    template <class C>
    void getComponentsInChildren ( const Unigine::NodePtr &node, Unigine::Vector<C*> &components )
    {
        ComponentSystem::get()->getComponentsInChildren<C> ( node, components );
    }

    template <class C>
    int removeComponent ( const Unigine::NodePtr &node )
    {
        return ComponentSystem::get()->removeComponent<C> ( node );
    }

    //////////////////////////////////////////////////////////
    // variables
    //////////////////////////////////////////////////////////

    // variables
    Unigine::NodePtr node;
    int property_num;

    //////////////////////////////////////////////////////////
    // functions
    //////////////////////////////////////////////////////////

#define VECTOR_OF_COMPONENT_FUNCTIONS(NAME) \
	Unigine::Map<int, Unigine::Vector<Unigine::CallbackBase*>> functions_##NAME; // <order, component's functions>
    VECTOR_OF_COMPONENT_FUNCTIONS ( init );
    VECTOR_OF_COMPONENT_FUNCTIONS ( updateAsyncThread );
    VECTOR_OF_COMPONENT_FUNCTIONS ( updateSyncThread );
    VECTOR_OF_COMPONENT_FUNCTIONS ( update );
    VECTOR_OF_COMPONENT_FUNCTIONS ( postUpdate );
    VECTOR_OF_COMPONENT_FUNCTIONS ( updatePhysics );
    VECTOR_OF_COMPONENT_FUNCTIONS ( swap );
    VECTOR_OF_COMPONENT_FUNCTIONS ( shutdown );
    VECTOR_OF_COMPONENT_FUNCTIONS ( destroyRenderResources );
#undef VECTOR_OF_COMPONENT_FUNCTIONS

private:
    // initialization
    void null_check();
    void initialize();
    void init_called();

    void refresh_enabled();

    // id of the node when component was created
    // (user can change id at runtime)
    int node_id;

    // init() method was called flag
    int initialized;
    int init_calls_count;
    int all_init_called;

    // shutdown() method was called flag
    int shutdown_called;

    int enabled_self = 0;
    int enabled = 0;
    int invoke_disabled_all_init_methods = 0;

    // callbacks
    Unigine::CallbackBase *destroy_callback;
};

// base class for the structs inside components
class ComponentStruct: public PropertyWrapper
{
public:
    ComponentStruct() = default;
    virtual ~ComponentStruct() = default;
};

#ifndef __GNUC__
#pragma endregion Base
#endif
