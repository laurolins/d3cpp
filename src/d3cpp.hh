#pragma once

#include <iostream>
#include <vector>
#include <memory>
#include <functional>
#include <map>
#include <string>
#include <deque>

/*! \brief d3 data driven documents selection mechanism for C++
 *
 * Generic mechanism to append, remove, update "elements" in
 * a "tree" document. The "tree" document can be user defined
 */

namespace d3cpp {
    
    //------------------------------------------------------------------------------
    // ElementValue
    //------------------------------------------------------------------------------
    
    template <typename E, typename T>
    struct ElementValue {
        ElementValue() = default;
        ElementValue(E* element);
        
        ElementValue(E* element, T value);
        
        E *element { nullptr };
        T value;
    };
    
    //------------------------------------------------------------------------------
    // Group
    //------------------------------------------------------------------------------
    
    template <typename E, typename T>
    struct Group {
        
        using element_value_type = ElementValue<E, T>;
        
        Group() = default;
        Group(E* parent_node);
        Group(E* parent_node, E* single);
        
        Group& add(E* e, T value);
        Group& add(E* e);
        
        E* parent_node;
        std::vector<element_value_type> elements;
    };
    
    
    template <typename E, typename T>
    struct EnterSelection;
    
    template <typename E, typename T>
    struct ExitSelection;
    
    //------------------------------------------------------------------------------
    // Selection
    //------------------------------------------------------------------------------
    
    // two types are associated with a selection: the enter/update type
    // and the exit type
    
    template <typename E, typename T>
    struct Selection {
    public:
        
        using element_type         = E;
        using data_type            = T;
        using selection_type       = Selection;
        using enter_selection_type = EnterSelection<E, T>;
        using exit_selection_type  = ExitSelection<E, T>;
        using group_type           = Group<E, T>;
        
        using predicate_type       = std::function<bool(const E*)>;
        using append_function_type = std::function<E*(E*)>;
        using call_type            = std::function<void(E*, T)>;

        using remove_from_document_function_type = std::function<void(E*)>;
        using remove_from_group_function_type = std::function<void(remove_from_document_function_type)>;
        
    public:
        
        Selection() = default;
        
        Selection(E *element);
        
        group_type& _group_add(E *parent_node, E* new_child);
        group_type& _group_add(E *parent_node);
        
        template <typename U>
        Selection<E,U> data(const std::vector<U>& data);
        
        
        // attr and append should be abstracted to applying a function...
        // Selection<E,T>& attr(const std::string &key,  std::function<std::string(T,int)> f);
        selection_type        append(append_function_type a);
        
        template <typename I> // can add additional constraint of how to search for children
        selection_type        selectAll(predicate_type p, std::function<I(E*)> gen_iterator);
        
        enter_selection_type& enter();
        selection_type&       exit();
        
        selection_type&       call(call_type f);
        
        void remove(remove_from_document_function_type remove_from_document)
        
    public:
        void _enterSelection_init(const std::vector<T>& extra_data);
        void _enterSelection_add(group_type* main_selection_group, int index);
        
    public:
        void _exitSelection_init();
        void _exitSelection_add(remove_from_group_function_type remove_from_group_function);
        
    public:
        std::vector<std::unique_ptr<group_type>> groups;
        std::unique_ptr<enter_selection_type> enter_selection;
        std::unique_ptr<exit_selection_type>  exit_selection;
    };
    
    
    //------------------------------------------------------------------------------
    // Document
    //------------------------------------------------------------------------------
    
    template <typename E>
    struct Document {
        using predicate_type       = std::function<bool(const E*)>;
        using selection_type       = Selection<E,int>;
        
    public:
        Document() = default;
        Document(E* root);
        
        template <typename I> // can add additional constraint of how to search for children
        selection_type selectAll(predicate_type p, std::function<I(E*)> gen_iterator);
        
        E *root { nullptr };
    };
    
    //------------------------------------------------------------------------------
    // EnterSelection
    //------------------------------------------------------------------------------
    
    template <typename E, typename T>
    struct EnterSelection {
        
        using group_type = Group<E, T>;
        using selection_type = Selection<E, T>;
        using enter_selection_type = EnterSelection<E, T>;
        using append_function_type = std::function<E*(E*)>;
        
        struct Entry {
            Entry() = default;
            Entry(group_type* group, int index);
            group_type*  group;
            int          index;
        };
        
        EnterSelection(selection_type *update_selection, const std::vector<T> &enter_data);
        
        enter_selection_type& add(group_type* update_selection_group, int index);
        selection_type        append(append_function_type a);
        
    public:
        selection_type *update_selection;
        std::vector<Entry> entries;
        std::vector<T>     enter_data;
    };
    
    //------------------------------------------------------------------------------
    // ExitSelection
    //------------------------------------------------------------------------------
    
    template <typename E, typename T>
    struct ExitSelection {
        
        using remove_from_document_function_type = std::function<void(E*)>;
        using remove_from_group_function_type    = std::function<void(remove_from_document_function_type)>;
        using group_type                         = Group<E,T>;
        using selection_type                     = Selection<E,T>;
        using exit_selection_type                = ExitSelection<E,T>;
        
        struct Entry {
            Entry() = default;
            Entry(remove_from_group_function_type remove_from_group_function);
            remove_from_group_function_type  remove_from_group_function;
        };
        
        ExitSelection(selection_type *update_selection);
        
        ExitSelection& add(remove_from_group_function_type f);
        
        void remove(remove_from_document_function_type f);
        
        selection_type *update_selection;
        std::vector<Entry> entries;
        
        // let's do by index first
    };
    
    
    
    
    //------------------------------------------------------------------------------
    // ElementValue Impl.
    //------------------------------------------------------------------------------
    
    template <typename E, typename T>
    ElementValue<E,T>::ElementValue(E* element):
    element(element)
    {}
    
    template <typename E, typename T>
    ElementValue<E,T>::ElementValue(E* element, T value):
    element(element),
    value(value)
    {}
    
    //------------------------------------------------------------------------------
    // Group Impl.
    //------------------------------------------------------------------------------
    
    template <typename E, typename T>
    Group<E,T>::Group(E* parent_node):
    parent_node(parent_node)
    {}
    
    template <typename E, typename T>
    Group<E,T>::Group(E* parent_node, E* single):
    parent_node(parent_node)
    {
        elements.push_back({single});
    }
    
    template<typename E, typename T>
    auto Group<E,T>::add(E* e, T value) -> Group& {
        elements.push_back({e,value});
        return *this;
    }
    
    template<typename E, typename T>
    auto Group<E,T>::add(E* e) -> Group& {
        elements.push_back({e});
        return *this;
    }
    
    //------------------------------------------------------------------------------
    // Selection Impl.
    //------------------------------------------------------------------------------
    
    template <typename E, typename T>
    Selection<E,T>::Selection(E* element)
    {
        groups.push_back(std::unique_ptr<group_type>(new group_type(element)));
    }
    
    template <typename E, typename T>
    auto Selection<E,T>::_group_add(E *parent_node, E* new_child) -> group_type& {
        groups.push_back(std::unique_ptr<group_type>(new group_type {parent_node, new_child}));
        return *groups.back().get();
    }
    
    template <typename E, typename T>
    auto Selection<E,T>::_group_add(E *parent_node) -> group_type& {
        groups.push_back(std::unique_ptr<group_type>(new group_type {parent_node}));
        return *groups.back().get();
    }
    
    template <typename E, typename T>
    auto Selection<E,T>::append(append_function_type append_function) -> selection_type
    {
        selection_type result;
        for (auto &g: groups) {
            auto new_child_element = append_function(g->parent_node);
            result._group_add(g->parent_node, new_child_element);
        }
        return result;
    }
    
    template <typename E, typename T>
    template <typename U>
    Selection<E,U> Selection<E,T>::data(const std::vector<U>& data) {
        Selection<E,U> result;
        
        // just the bare update part here... not enter or exit
        // match by index
        
        result._enterSelection_init(data);
        result._exitSelection_init();
        
        Selection<E,U> exit_selection;
        
        for (auto &g: groups) {
            
            auto &new_group = result._group_add(g->parent_node);
            
            auto it_data  = data.begin();
            auto it_ev    = g->elements.begin();
            auto index = 0;
            for (;it_data!= data.end() && it_ev!=g->elements.end();++it_data,++it_ev) {
                new_group.add(it_ev->element, *it_data);
                ++index;
            }
            
            //
            // this is still wrong
            //
            
            result._enterSelection_add(&new_group,index);
            
            auto original_group = g.get();
            std::cerr << "registering exit selection on original_group: " << original_group << " size: " << original_group->elements.size() << std::endl;
            
            using ev_type = decltype(g->elements[0]);
            if (it_ev != g->elements.end()) {
                auto &exit_group = exit_selection._group_add({g->parent_node});
                std::for_each(it_ev, g->elements.end(), [&exit_group](const ev_type &ev) {
                    exit_group.add(ev.element);
                });
            }
            
            //
            // if (it_ev != e->elements.end()) {
            //    result.setExitSelection(std::vector<U>(it_ev,))
            // }
            //
            
        }
        
        return result;
    }
    
    //template <typename E, typename T>
    //Selection<E,T>& Selection<E,T>::attr(const std::string &key,  std::function<std::string(T,int)> f) {
    //    for (auto &g: groups) {
    //        int index = 0;
    //        for (auto &it_ev: g->elements) {
    //            it_ev.element->attr(key, f(it_ev.value,index));
    //            ++index;
    //        }
    //    }
    //    return *this;
    //}
    
    template<typename E, typename T>
    void Selection<E,T>::_enterSelection_init(const std::vector<T>& extra_data) {
        enter_selection.reset(new enter_selection_type(this,extra_data));
    }
    
    template<typename E, typename T>
    void Selection<E,T>::_enterSelection_add(group_type *main_selection_parent_children, int index) {
        if (!enter_selection)
            throw std::runtime_error("ooops");
        enter_selection->add(main_selection_parent_children,index);
    }
    
    template<typename E, typename T>
    void Selection<E,T>::_exitSelection_init() {
        exit_selection.reset(new exit_selection_type(this));
    }
    
    template<typename E, typename T>
    void Selection<E,T>::_exitSelection_add(remove_from_group_function_type remove_from_group_function) {
        if (!enter_selection)
            throw std::runtime_error("ooops");
        exit_selection->add(remove_from_group_function);
    }
    
    
    template<typename E, typename T>
    auto Selection<E,T>::enter() -> enter_selection_type& {
        return *enter_selection.get();
    }
    
    template<typename E, typename T>
    auto Selection<E,T>::exit() -> exit_selection_type& {
        return *exit_selection.get();
    }
    
    template<typename E, typename T>
    template<typename I>
    auto Selection<E,T>::selectAll(predicate_type predicate, std::function<I(E*)> gen_iterator) -> selection_type {
        selection_type result;
        for (auto &group: groups) {
            for (auto &ev: group->elements) {
                auto it = gen_iterator(ev.element);
                group_type& g = result._group_add(ev.element);
                while (auto e = it.next()) {
                    if (predicate(e)) {
                        g.add(e);
                    }
                }
            }
        }
        return result;
    }
    
    
    template<typename E, typename T>
    auto Selection<E,T>::call(call_type f) -> selection_type& {
        for (auto &group: groups) {
            for (auto &ev: group->elements) {
                f(ev.element, ev.value);
            }
        }
        return *this;
    }
    
    template<typename E, typename T>
    std::ostream& operator<<(std::ostream &os, const Selection<E,T>& sel) {
        os << "[selection]" << std::endl;
        for (auto &g: sel.groups) {
            os << "    [group]" << std::endl;
            os << "        [parent_node]  <" << ">" << std::endl;
            for (auto &ev: g->elements) {
                os << "            [element]  <" << ">" << std::endl;
            }
        }
        return os;
    }
    
    
    //------------------------------------------------------------------------------
    // EnterSelection::Entry Impl.
    //------------------------------------------------------------------------------
    
    template <typename E, typename T>
    EnterSelection<E,T>::Entry::Entry(group_type* group, int index):
    group(group),
    index(index)
    {}
    
    //------------------------------------------------------------------------------
    // EnterSelection Impl.
    //------------------------------------------------------------------------------
    
    template <typename E, typename T>
    EnterSelection<E,T>::EnterSelection(selection_type *update_selection, const std::vector<T> &enter_data):
    update_selection(update_selection),
    enter_data(enter_data)
    {}
    
    template <typename E, typename T>
    auto EnterSelection<E,T>::add(group_type* update_selection_group, int index) -> enter_selection_type& {
        entries.push_back({update_selection_group, index});
        return *this;
    }
    
    template <typename E, typename T>
    auto EnterSelection<E,T>::append(append_function_type append) -> selection_type {
        selection_type result;
        for (auto &e: entries) {
            auto &new_group = result._group_add(e.group->parent_node);
            for (auto it=enter_data.begin() + e.index;it!=enter_data.end();++it) {
                auto new_element = append(e.group->parent_node);
                new_group.add(new_element, *it);
                e.group->add(new_element, *it);
            }
        }
        return result;
    }
    
    
    //--------------------------------------------------------------------------------------
    // ExitSelection Impl.
    //--------------------------------------------------------------------------------------
    
    template <typename E, typename T>
    ExitSelection<E,T>::Entry::Entry(remove_from_group_function_type remove_from_group_function):
    remove_from_group_function(remove_from_group_function)
    {}
    
    template <typename E, typename T>
    ExitSelection<E,T>::ExitSelection(selection_type *update_selection):
    update_selection(update_selection)
    {}
    
    template <typename E, typename T>
    auto ExitSelection<E,T>::add(remove_from_group_function_type remove_from_group_function) -> exit_selection_type& {
        entries.push_back({remove_from_group_function});
        return *this;
    }
    
    template <typename E, typename T>
    void ExitSelection<E,T>::remove(remove_from_document_function_type remove_from_document_function) {
        for (auto &e: entries) {
            e.remove_from_group_function(remove_from_document_function);
        }
    }
    
    //------------------------------------------------------------------------------
    // Document Impl.
    //------------------------------------------------------------------------------
    
    template <typename E>
    Document<E>::Document(E* root):
    root(root)
    {}
    
    template <typename E>
    template<typename I>
    auto Document<E>::selectAll(predicate_type predicate, std::function<I(E*)> gen_iterator) -> selection_type {
        
        if (!root)
            throw std::runtime_error("oooops");
        
        selection_type result; // int is the default placeholder for data
        auto &group = result._group_add(root);
        
        auto it = gen_iterator(root);
        while (auto e = it.next()) {
            if (predicate(e)) {
                group.add(e);
            }
        }
        return result;
    }
    
} // d3cpp



