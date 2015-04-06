#include <iostream>
#include <vector>
#include <memory>
#include <functional>
#include <map>
#include <string>
#include <deque>

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
// ElementIterator
//------------------------------------------------------------------------------

struct ElementIterator {
    struct Item {
        Item() = default;
        Item(Element *element, int depth);
        Element* element { nullptr };
        int depth;
    };
    
    static const int UNBOUNDED = -1;
    
    ElementIterator(int max_depth=UNBOUNDED);
    ElementIterator(Element *root, int max_depth=UNBOUNDED);
    void push(Element *e); // level zero

    Element* next();
    
    std::deque<Item> stack;
    int max_depth { UNBOUNDED }; // indicates any level
};


//------------------------------------------------------------------------------
// ElementValue
//------------------------------------------------------------------------------

template <typename T>
struct ElementValue {
    ElementValue() = default;
    ElementValue(Element* element);
    
    ElementValue(Element* element, T value);
    
    Element *element { nullptr };
    T value;
};

//------------------------------------------------------------------------------
// Group
//------------------------------------------------------------------------------

template <typename T>
struct Group {
    
    using element_value_type = ElementValue<T>;
    
    Group() = default;
    Group(Element* parent_node);
    Group(Element* parent_node, Element* single);
    
    Group& append(Element* e, T value);
    Group& append(Element* e);
    
    Element* parent_node;
    std::vector<element_value_type> elements;
};


template <typename T>
struct EnterSelection;

//------------------------------------------------------------------------------
// Selection
//------------------------------------------------------------------------------

template <typename T>
struct Selection {
public:

    using group_type = Group<T>;
    
public:
    
    Selection() = default;
    
    Selection(Element *element);
    
    group_type& _group_add(Element *parent_node, Element* new_child);
    group_type& _group_add(Element *parent_node);
    
    template <typename U>
    Selection<U> data(const std::vector<U>& data);
    Selection<T>& attr(const std::string &key,  std::function<std::string(T,int)> f);
    Selection append(const std::string& tag);
    
    EnterSelection<T>& enter();
    
    Selection<T> selectAll(const std::string &tag);
    
public:
    void _enterSelection_init(const std::vector<T>& extra_data);
    void _enterSelection_add(group_type* main_selection_parent_children, int index);
public:
    std::vector<std::unique_ptr<group_type>> groups;
    std::unique_ptr<EnterSelection<T>> enter_selection;
};


//------------------------------------------------------------------------------
// Document
//------------------------------------------------------------------------------

struct Document {
    Document() = default;
    Document(Element* root);
    
    Selection<int> selectAll(const std::string &tag);

    Element *root { nullptr };
};

//------------------------------------------------------------------------------
// EnterSelection
//------------------------------------------------------------------------------


template <typename T>
struct EnterSelection {

    using group_type = Group<T>;
    
    struct Entry {
        Entry() = default;
        Entry(group_type* group, int index);
        group_type*  group;
        int          index;
    };
    
    EnterSelection(Selection<T> *update_selection, const std::vector<T> &enter_data);
    
    EnterSelection& add(group_type* update_selection_parent_children, int index);
    
    Selection<T> append(const std::string& tag);
    Selection<T> *update_selection;

    std::vector<Entry> entries;
    std::vector<T>     enter_data; // other data was already mapped
    
    // let's do by index first
};



























//------------------------------------------------------------------------------
// Document Impl.
//------------------------------------------------------------------------------

Document::Document(Element* root):
root(root)
{}

Selection<int> Document::selectAll(const std::string &tag) {
    Selection<int> result;
    if (!root)
        throw std::runtime_error("oops");
    
    auto &group = result._group_add(root);
    
    ElementIterator it(root);
    while (auto e=it.next()) {
        if (e->tag.compare(tag) == 0) {
            group.append(e);
        }
    }
    return result;
}

//------------------------------------------------------------------------------
// EnterSelection::Entry Impl.
//------------------------------------------------------------------------------

template <typename T>
EnterSelection<T>::Entry::Entry(group_type* parent_children, int index):
group(group),
index(index)
{}

//------------------------------------------------------------------------------
// EnterSelection Impl.
//------------------------------------------------------------------------------

template <typename T>
EnterSelection<T>::EnterSelection(Selection<T> *update_selection, const std::vector<T> &enter_data):
    update_selection(update_selection),
    enter_data(enter_data)
{}


template <typename T>
EnterSelection<T>& EnterSelection<T>::add(group_type* update_selection_parent_children, int index) {
    entries.push_back({update_selection_parent_children, index});
    return *this;
}

template <typename T>
Selection<T> EnterSelection<T>::append(const std::string& tag) {
    Selection<T> result;
    for (auto &e: entries) {
        auto &new_group = result._group_add(e.group->parent_node);
        for (auto it=enter_data.begin() + e.index;it!=enter_data.end();++it) {
            auto &new_element = e.group->parent_node->append(tag);
            new_group.append(&new_element, *it);
            e.group->append(&new_element, *it);
        }
    }
    return result;
}

//------------------------------------------------------------------------------
// ElementValue Impl.
//------------------------------------------------------------------------------

template <typename T>
ElementValue<T>::ElementValue(Element* element):
    element(element)
{}

template <typename T>
ElementValue<T>::ElementValue(Element* element, T value):
element(element),
value(value)
{}

//------------------------------------------------------------------------------
// Group Impl.
//------------------------------------------------------------------------------

template <typename T>
Group<T>::Group(Element* parent_node):
parent_node(parent_node)
{}

template <typename T>
Group<T>::Group(Element* parent_node, Element* single):
    parent_node(parent_node)
{
    elements.push_back({single});
}

template<typename T>
auto Group<T>::append(Element* e, T value) -> Group<T>& {
    elements.push_back({e,value});
    return *this;
}

template<typename T>
auto Group<T>::append(Element* e) -> Group<T>& {
    elements.push_back({e});
    return *this;
}

//------------------------------------------------------------------------------
// Selection Impl.
//------------------------------------------------------------------------------

template <typename T>
Selection<T>::Selection(Element* element)
{
    groups.push_back(std::unique_ptr<group_type>(new group_type(element)));
}

template <typename T>
auto Selection<T>::_group_add(Element *parent_node, Element* new_child) -> group_type& {
    groups.push_back(std::unique_ptr<group_type>(new group_type {parent_node, new_child}));
    return *groups.back().get();
}

template <typename T>
auto Selection<T>::_group_add(Element *parent_node) -> group_type& {
    groups.push_back(std::unique_ptr<group_type>(new group_type {parent_node}));
    return *groups.back().get();
}

template <typename T>
Selection<T> Selection<T>::append(const std::string& tag)
{
    Selection<T> result;
    for (auto &g: groups) {
        auto &node = g->parent_node->append(tag);
        result._group_add(g->parent_node, &node);
    }
    return result;
}

template <typename T>
template <typename U>
Selection<U> Selection<T>::data(const std::vector<U>& data) {
    Selection<U> result;
    
    // just the bare update part here... not enter or exit
    // match by index
    
    result._enterSelection_init(data);
    
    for (auto &g: groups) {
        
        auto &new_group = result._group_add(g->parent_node);

        auto it_data  = data.begin();
        auto it_ev    = g->elements.begin();
        auto index = 0;
        for (;it_data!= data.end() && it_ev!=g->elements.end();++it_data,++it_ev) {
            new_group.append(it_ev->element, *it_data);
            ++index;
        }

        //
        // this is still wrong
        //
        
        if (it_data != data.end()) {
            result._enterSelection_add(&new_group,index);
        }
  
        //
        // if (it_ev != e->elements.end()) {
        //    result.setExitSelection(std::vector<U>(it_ev,))
        // }
        //
        
    }
    
    return result;
}

template <typename T>
Selection<T>& Selection<T>::attr(const std::string &key,  std::function<std::string(T,int)> f) {
    for (auto &g: groups) {
        int index = 0;
        for (auto &it_ev: g->elements) {
            it_ev.element->attr(key, f(it_ev.value,index));
            ++index;
        }
    }
    return *this;
}

template<typename T>
void Selection<T>::_enterSelection_init(const std::vector<T>& extra_data) {
    enter_selection.reset(new EnterSelection<T>(this,extra_data));
}

template<typename T>
void Selection<T>::_enterSelection_add(group_type *main_selection_parent_children, int index) {
    if (!enter_selection)
        throw std::runtime_error("ooops");
    enter_selection->add(main_selection_parent_children,index);
}


template<typename T>
EnterSelection<T>& Selection<T>::enter() {
    return *enter_selection.get();
}

template<typename T>
Selection<T> Selection<T>::selectAll(const std::string &tag) {
    Selection<T> result;
    for (auto &group: groups) {
        for (auto &ev: group->elements) {
            ElementIterator it(ev.element);
            group_type& g = result._group_add(ev.element);
            while (auto e = it.next()) {
                if (e->tag.compare(tag) == 0) {
                    g.append(e);
                }
            }
        }
    }
    return result;
}

template<typename T>
std::ostream& operator<<(std::ostream &os, const Selection<T>& sel) {
    os << "[selection]" << std::endl;
    for (auto &g: sel.groups) {
        
        os << "    [group]" << std::endl;
        os << "        [parent_node] <" << g->parent_node->tag << ">" << std::endl;
        for (auto &ev: g->elements) {
            os << "            [element] " << ev.element->tag << ">" << std::endl;
        }
    }
    return os;
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

//------------------------------------------------------------------------------
// ElementIterator Impl.
//------------------------------------------------------------------------------


ElementIterator::Item::Item(Element *element, int depth):
element(element),
depth(depth)
{}

ElementIterator::ElementIterator(int max_depth):
max_depth(max_depth)
{}

ElementIterator::ElementIterator(Element *root, int max_depth):
    max_depth(max_depth)
{
    stack.push_back({root,0});
}

void ElementIterator::push(Element* e) {
    stack.push_back({e,0});
}

Element* ElementIterator::next() {
    
    if (stack.empty())
        return nullptr;
    
    Item item = stack.front();
    stack.pop_front();
    
    // schedule processing of childrens
    if (max_depth != UNBOUNDED && item.depth < max_depth) {
        for (auto it=item.element->children.rbegin();it!=item.element->children.rend();++it) {
            stack.push_back( {it->get(), item.depth+1} );
        }
    }
    
    return item.element;
    
}



//------------------------------------------------------------------------------
// main
//------------------------------------------------------------------------------

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
    
    Document document(&root);
    
    std::cout << document.selectAll("a");

    auto update_sel = document.selectAll("a").data(points);

    std::cout << document.selectAll("a").data(points);

    update_sel.enter().append("a")
       .attr("x",[](const Point& p, int i) { return std::to_string(p.x); })
       .attr("y",[](const Point& p, int i) { return std::to_string(p.y); });

    update_sel
    .attr("x",[](const Point& p, int i) { return std::to_string(p.y); })
    .attr("y",[](const Point& p, int i) { return std::to_string(p.x); });
    
    std::cout << document.selectAll("a");

    document.selectAll("a").selectAll("b").data(points).enter().append("b");
    
//    data(points).enter().append("b");

    
    
    
    std::cout << root;
    
}

