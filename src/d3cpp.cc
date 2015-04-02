#include <iostream>
#include <vector>
#include <memory>
#include <functional>
#include <map>
#include <string>


//------------------------------------------------------------------------------
// Any
//------------------------------------------------------------------------------

namespace any {

    //
    // ValueBase
    //
    struct ValueBase {
        ValueBase() = default;
        virtual ~ValueBase() {}
        virtual ValueBase* clone() = 0;
    };
    
    //
    // Value
    //
    template <typename T>
    struct Value: public ValueBase {
        Value(const T& value);
        virtual ValueBase* clone();
        ~Value();
    public:
        T value;
    };

    //
    // Variant object
    //
    
    struct Any {
    public:
        Any() = default;
        
        template <typename T>
        Any(const T &value);
        
        Any(Any &&other);
        Any(const Any &other);
        Any& operator=(const Any &other);
        Any& operator=(Any &&other);
        
    public:
        std::unique_ptr<ValueBase> content;
    };
    
    template <typename T>
    T& any_cast(const Any &any);


}

//------------------------------------------------------------------------------
// Element
//------------------------------------------------------------------------------

struct Element {
public:
    Element() = default;
    Element(const std::string& tag, Element* parent=nullptr);
    Element& append(const std::string &tag);
    Element& attr(const std::string& key, const std::string& value);
    const std::string& attr(const std::string &key);
public:
    std::string tag;
    Element*    parent {nullptr};
    std::vector<std::unique_ptr<Element>> children;
    std::map<std::string, std::string> attributes;
};













//------------------------------------------------------------------------------
// ElementValue
//------------------------------------------------------------------------------

struct ElementValue {
    ElementValue() = default;
    ElementValue(Element* element);
    
    template <typename T>
    ElementValue(Element* element, T value);
    
    Element *element { nullptr };
    any::Any value;
};

//------------------------------------------------------------------------------
// SelectionEntry
//------------------------------------------------------------------------------

struct SelectionEntry {
    SelectionEntry() = default;
    SelectionEntry(Element* parent_node);
    SelectionEntry(const ElementValue &parent_node, Element* single);
    SelectionEntry(const ElementValue &parent_node);
    
    template<typename T>
    SelectionEntry& append(Element* e, T value);
    
    ElementValue parent_node;
    std::vector<ElementValue> elements;
};


template <typename T>
struct EnterSelection;

//------------------------------------------------------------------------------
// Selection
//------------------------------------------------------------------------------

template <typename T>
struct Selection {
    Selection() = default;
    
    Selection(Element *element);
    
    
    SelectionEntry& add(const ElementValue& root, Element* new_child);
    SelectionEntry& add(const ElementValue& ev);
    
    template <typename U>
    Selection<U> data(const std::vector<U>& data);
    Selection<T>& attr(const std::string &key,  std::function<std::string(T,int)> f);
    Selection append(const std::string& tag);
    
    EnterSelection<T>& enter();
    
public:
    void setEnterSelection(const std::vector<T>& extra_data);
public:
    std::vector<std::unique_ptr<SelectionEntry>> entries;
    std::unique_ptr<EnterSelection<T>> enter_selection;
};


//------------------------------------------------------------------------------
// EnterSelection
//------------------------------------------------------------------------------

template <typename T>
struct EnterSelection {
    EnterSelection(Selection<T> *update_selection, const std::vector<T> &object);
    Selection<T> append(const std::string& tag);
    Selection<T> *update_selection;
    std::vector<T> enter_data; // other data was already mapped
};

template <typename T>
EnterSelection<T>::EnterSelection(Selection<T> *update_selection, const std::vector<T> &enter_data):
    update_selection(update_selection),
    enter_data(enter_data)
{}

template <typename T>
Selection<T> EnterSelection<T>::append(const std::string& tag) {
    Selection<T> result;
    for (auto &e: update_selection->entries) {
        auto &tree = result.add(e->parent_node);
        for (auto &obj: enter_data) {
            auto &new_element = e->parent_node.element->append(tag);
            tree.append(&new_element, obj);
            e.get()->append(&new_element, obj);
        }
    }
    return result;
}

//------------------------------------------------------------------------------
// ElementValue Impl.
//------------------------------------------------------------------------------

ElementValue::ElementValue(Element* element):
    element(element)
{}

template <typename T>
ElementValue::ElementValue(Element* element, T value):
element(element),
value(value)
{}

//------------------------------------------------------------------------------
// SelectionEntry Impl.
//------------------------------------------------------------------------------

SelectionEntry::SelectionEntry(Element* parent_node):
parent_node(parent_node)
{}

SelectionEntry::SelectionEntry(const ElementValue &parent_node, Element* single):
    parent_node(parent_node)
{
    elements.push_back({single});
}

SelectionEntry::SelectionEntry(const ElementValue &parent_node):
parent_node(parent_node)
{}

template<typename T>
SelectionEntry& SelectionEntry::append(Element* e, T value) {
    elements.push_back({e,value});
    return *this;
}

//------------------------------------------------------------------------------
// Selection Impl.
//------------------------------------------------------------------------------

template <typename T>
Selection<T>::Selection(Element* element)
{
    entries.push_back(std::unique_ptr<SelectionEntry>(new SelectionEntry(element)));
}

template <typename T>
SelectionEntry& Selection<T>::add(const ElementValue& root, Element* new_child) {
    entries.push_back(std::unique_ptr<SelectionEntry>(new SelectionEntry {root, new_child}));
    return *entries.back().get();
}

template <typename T>
SelectionEntry& Selection<T>::add(const ElementValue& ev) {
    entries.push_back(std::unique_ptr<SelectionEntry>(new SelectionEntry {ev}));
    return *entries.back().get();
}

template <typename T>
Selection<T> Selection<T>::append(const std::string& tag)
{
    Selection<T> result;
    for (auto &it: entries) {
        auto &node = it->parent_node.element->append(tag);
        result.add(it->parent_node, &node);
    }
    return result;
}

template <typename T>
template <typename U>
Selection<U> Selection<T>::data(const std::vector<U>& data) {
    Selection<U> result;
    
    // just the bare update part here... not enter or exit
    // match by index
    
    for (auto &e: entries) {
        
        auto &tree = result.add(e->parent_node);

        auto it_data  = data.begin();
        auto it_ev    = e->elements.begin();
        for (;it_data!= data.end() && it_ev!=e->elements.end();++it_data,++it_ev) {
            tree.append(it_ev->element, *it_data);
        }
        if (it_data != data.end()) {
            result.setEnterSelection(std::vector<U>(it_data,data.end()));
        }
    }
    
    return result;
}

template <typename T>
Selection<T>& Selection<T>::attr(const std::string &key,  std::function<std::string(T,int)> f) {
    for (auto &e: entries) {
        int index = 0;
        for (auto &it_ev: e->elements) {
            auto value = any::any_cast<T>(it_ev.value);
            it_ev.element->attr(key, f(value,index));
            ++index;
        }
    }
    return *this;
}

template<typename T>
void Selection<T>::setEnterSelection(const std::vector<T>& data) {
    enter_selection.reset(new EnterSelection<T>(this,data));
}

template<typename T>
EnterSelection<T>& Selection<T>::enter() {
    return *enter_selection.get();
}


//-------------------------------------------------------------------------------
// any Impl.
//-------------------------------------------------------------------------------

namespace any {
    
    //
    // Impl. of Value
    //
    
    template <typename T>
    Value<T>::Value(const T& value):
    value { value } // a copy
    {}
    
    template <typename T>
    ValueBase* Value<T>::clone() {
        return new Value<T>(value);
    }
    
    template <typename T>
    Value<T>::~Value() {}
    
    //
    // Any Impl.
    //
    
    template <typename T>
    Any::Any(const T &value) {
        content.reset(new Value<T>(value));
    }
    
    Any::Any(Any &&other)
    {
        content.swap(other.content);
    }
    
    Any::Any(const Any &other)
    {
        if (other.content)
            content.reset(other.content->clone());
    }
    
    Any& Any::operator=(const Any &other)
    {
        if (other.content)
            content.reset(other.content->clone());
        else
            content.reset();
        return *this;
    }
    
    Any& Any::operator=(Any &&other)
    {
        content.swap(other.content);
        return *this;
    }
    
    template <typename T>
    T& any_cast(const Any &any) {
        auto p = dynamic_cast<Value<T>*>(any.content.get());
        if (!p)
            throw std::runtime_error("ooops");
        return p->value;
    }
    
}

//------------------------------------------------------------------------------
// Element Impl.
//------------------------------------------------------------------------------

Element::Element(const std::string& tag, Element* parent):
tag(tag),
parent(parent)
{}

Element& Element::append(const std::string &tag) {
    children.push_back(std::unique_ptr<Element>(new Element(tag,this)));
    return *children.back().get();
}

Element& Element::attr(const std::string& key, const std::string& value) {
    attributes[key] = value;
    return *this;
}

const std::string& Element::attr(const std::string &key) {
    return attributes[key];
}


std::ostream& operator<<(std::ostream &os, const Element& e) {
    
    std::function<void(const Element& e, int level)> print =  [&os, &print](const Element& e, int level) {
        
        std::string prefix(level*4, ' ');
        
        if (!e.children.size()) {
            os << prefix << "<" << e.tag;
            for (auto it: e.attributes) {
                os <<  " " << it.first << "=\"" << it.second << "\"";
            }
            os << "/>" << std::endl;
        }
        else {
            os << prefix << "<" << e.tag;
            for (auto it: e.attributes) {
                os <<  " " << it.first << "=\"" << it.second << "\"";
            }
            os << ">" << std::endl;
            for (auto &c: e.children) {
                print(*c.get(), level + 1);
            }
            os << prefix << "</" << e.tag << ">" << std::endl;
        }
    };
    
    print(e, 0);
    
    return os;
}






int main() {
    
    struct Point {
        Point() = default;
        Point(int x, int y):
            x(x), y(y)
        {}
        int x;
        int y;
    };
    
    std::vector<Point> points { {1,7}, {6,9}, {10,11} };
    
    Element root("root");
    
    Selection<int> sel(&root);

    
    
//    sel.append("a").append("b").data(points)
//    .attr("x",[](const Point& p, int i) { return std::to_string(p.x); })
//    .attr("y",[](const Point& p, int i) { return std::to_string(p.y); });

    auto update_sel = sel.data(points);
    
    update_sel.enter().append("a")
       .attr("x",[](const Point& p, int i) { return std::to_string(p.x); })
       .attr("y",[](const Point& p, int i) { return std::to_string(p.y); });

    update_sel
    .attr("x",[](const Point& p, int i) { return std::to_string(p.y); })
    .attr("y",[](const Point& p, int i) { return std::to_string(p.x); });

    
    
    
//    sel.data(points)
//       .append("node")
//          .attr("x",[](const Point& p, int i) { return std::to_string(p.x); })
//          .attr("y",[](const Point& p, int i) { return std::to_string(p.y); })
//          .data(points)
//          .append("deeper-node")
//             .attr("x",[](const Point& p, int i) { return std::to_string(p.x); })
//             .attr("y",[](const Point& p, int i) { return std::to_string(p.y); });


    
    std::cout << root;
    
}










































#if 0

//------------------------------------------------------------------------------
// Forward Decl.
//------------------------------------------------------------------------------

template <typename T>
struct  SelectionData;

//------------------------------------------------------------------------------
// Selection
//------------------------------------------------------------------------------

enum SelectionMode { UPDATE, EXIT, ENTER };

struct Selection {
    Selection() = default;
    Selection(std::vector<Element>& list);
    
    Selection& add(Element *e);
    
    template <typename T>
    Selection& add(Element *e, T obj);
    
    template <typename T>
    SelectionData<T> data(std::vector<T> &data);
    
    Selection& value(std::function<void(Element&, int)>);
    
    void update();
    void exit();
    void enter();
    
    SelectionMode mode { UPDATE };
    std::vector<any::Any> join_data;     // data for joining
    std::vector<Element*> elements;
};

//------------------------------------------------------------------------------
// SelectionData
//------------------------------------------------------------------------------

template <typename T>
struct  SelectionData {
    
    SelectionData(Selection& selection); // existing selection
    SelectionData(Selection&& selection); // own a new selection
    
    SelectionData& attr(const std::string &key, std::function<std::string(T,int)>);
    SelectionData  append(const std::string &tag);
    
    Selection& selection();
    
    template <typename U>
    SelectionData<U> data(std::vector<U> &data);
    
    void update();
    void exit();
    void enter();
    
public:
    std::unique_ptr<Selection> own_selection;
    Selection                 *shared_selection { nullptr };
};



























//------------------------------------------------------------------------------
// Element Impl.
//------------------------------------------------------------------------------

Element::Element(const std::string& tag, Element* parent):
tag(tag),
parent(parent)
{}

Element& Element::append(const std::string &tag) {
    children.push_back(std::unique_ptr<Element>(new Element(tag,this)));
    return *children.back().get();
}

Element& Element::attr(const std::string& key, const std::string& value) {
    attributes[key] = value;
    return *this;
}

const std::string& Element::attr(const std::string &key) {
    return attributes[key];
}


std::ostream& operator<<(std::ostream &os, const Element& e) {
    
    std::function<void(const Element& e, int level)> print =  [&os, &print](const Element& e, int level) {
        
        std::string prefix(level*4, ' ');
        
        if (!e.children.size()) {
            os << prefix << "<" << e.tag;
            for (auto it: e.attributes) {
                os <<  " " << it.first << "=\"" << it.second << "\"";
            }
            os << "/>" << std::endl;
        }
        else {
            os << prefix << "<" << e.tag;
            for (auto it: e.attributes) {
                os <<  " " << it.first << "=\"" << it.second << "\"";
            }
            os << ">" << std::endl;
            for (auto &c: e.children) {
                print(*c.get(), level + 1);
            }
            os << prefix << "</" << e.tag << ">" << std::endl;
        }
    };
    
    print(e, 0);
    
    return os;
}


//------------------------------------------------------------------------------
// Selection Impl.
//------------------------------------------------------------------------------

Selection::Selection(std::vector<Element>& list) {
    elements.reserve(list.size());
    for (auto &e: list) {
        elements.push_back({&e});
    }
}

Selection& Selection::add(Element *e) {
    elements.push_back(e);
    return *this;
}

template <typename T>
Selection& Selection::add(Element *e, T obj) {
    if (elements.size() != join_data.size())
        throw std::runtime_error("oops");
    elements.push_back(e);
    join_data.push_back(any::Any(obj));
    return *this;
}

template <typename T>
SelectionData<T> Selection::data(std::vector<T> &data) {
    for (auto &obj: data) {
        join_data.push_back(any::Any(obj));
    }
    return SelectionData<T>(*this);
}

void Selection::update() {
    mode = UPDATE;
}

void Selection::exit() {
    mode = EXIT;
}

void Selection::enter() {
    mode = ENTER;
}


//------------------------------------------------------------------------------
// SelectionData Impl.
//------------------------------------------------------------------------------

template <typename T>
SelectionData<T>::SelectionData(Selection& selection):
shared_selection(&selection)
{}

template <typename T>
SelectionData<T>::SelectionData(Selection&& selection):
own_selection(std::unique_ptr<Selection>(new Selection(selection)))
{}

template <typename T>
void SelectionData<T>::update() {
    selection().update();
}

template <typename T>
void SelectionData<T>::exit() {
    selection().exit();
}

template <typename T>
void SelectionData<T>::enter() {
    selection().enter();
}

template <typename T>
Selection& SelectionData<T>::selection() {
    Selection *psel;
    if (shared_selection) {
        psel = shared_selection;
    }
    else if (own_selection) {
        psel = own_selection.get();
    }
    else {
        throw std::runtime_error("ooops");
    }
    return *psel;
}


template <typename T>
SelectionData<T>& SelectionData<T>::attr(const std::string &key, std::function<std::string(T,int)> f) {
    
    Selection &selection = this->selection();
    
    // mode is update
    auto it_data    = selection.join_data.begin();
    auto it_element = selection.elements.begin();
    int i=0;
    for (;it_data != selection.join_data.end() && it_element!=selection.elements.end();++it_data,++it_element) {
        //        std::cout << any::any_cast<T>(*it_data).x << std::endl;
        //        std::cout << any::any_cast<T>(*it_data).y << std::endl;
        auto value = f(any::any_cast<T>(*it_data), i);
        (*it_element)->attr(key, value);
        ++i;
    }
    return *this;
}

template <typename T>
SelectionData<T> SelectionData<T>::append(const std::string &tag) {
    // mode is update
    Selection sel;
    Selection &selection = this->selection();
    for (auto &it_selection: selection.elements) {
        for (auto &it_data: selection.join_data) {
            //            std::cout << any::any_cast<T>(it_data).x << std::endl;
            //            std::cout << any::any_cast<T>(it_data).y << std::endl;
            auto& new_element = it_selection->append(tag);
            sel.add(&new_element, any::any_cast<T>(it_data));
        }
    }
    return SelectionData<T>(std::move(sel));
}


template <typename T>
template <typename U>
SelectionData<U> SelectionData<T>::data(std::vector<U> &data) {
    Selection& selection = this->selection();
    Selection sel;
    for (auto e: selection.elements) {
        sel.add(e);
    }
    for (auto &obj: data) {
        sel.join_data.push_back(any::Any(obj));
    }
    return SelectionData<U>(std::move(sel));
}

#endif



































































