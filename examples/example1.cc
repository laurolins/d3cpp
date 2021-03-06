#include <iostream>
#include <vector>
#include <memory>
#include <functional>
#include <map>
#include <string>

#include "d3cpp.hh"


//------------------------------------------------------------------------------
// Element
//------------------------------------------------------------------------------

struct Element {
public:
    Element() = default;
    Element(const std::string& tag, Element* parent=nullptr, int parent_index=0);
    Element& append(const std::string &tag);
    Element& attr(const std::string& key, const std::string& value);
    const std::string& attr(const std::string &key) const;
    void remove();
public:
    std::string tag;
    Element*    parent {nullptr};
    int         parent_index;
    std::vector<std::unique_ptr<Element>> children; // might have nullptrs inside
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
    
    std::vector<Item> stack;
    int max_depth { UNBOUNDED }; // indicates any level
};




//------------------------------------------------------------------------------
// Element Impl.
//------------------------------------------------------------------------------

Element::Element(const std::string& tag, Element* parent, int parent_index):
tag(tag),
parent(parent),
parent_index(parent_index)
{}

void Element::remove() {
    parent->children[parent_index].reset();
}

Element& Element::append(const std::string &tag) {
    children.push_back(std::unique_ptr<Element>(new Element(tag,this,(int) children.size())));
    return *children.back().get();
}

Element& Element::attr(const std::string& key, const std::string& value) {
    attributes[key] = value;
    return *this;
}

const std::string& Element::attr(const std::string &key) const {
    return attributes.at(key);
}

std::ostream& operator<<(std::ostream &os, const Element& e) {
    
    std::function<void(const Element& e, int level)> print =  [&os, &print](const Element& e, int level) {
        
        std::string prefix(level*4, ' ');
        
        os << prefix << "<" << e.tag;
        for (auto it: e.attributes) {
            os <<  " " << it.first << "=\"" << it.second << "\"";
        }

        if (!e.children.size()) {
            os << "/>" << std::endl;
        }
        else {
            os << ">" << std::endl;
            for (auto &c: e.children) {
                if (c)
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
    
    Item item = stack.back();
    stack.pop_back();
    
    // schedule processing of childrens
    if (max_depth == UNBOUNDED || item.depth < max_depth) {
        for (auto it=item.element->children.rbegin();it!=item.element->children.rend();++it) {
            if (it->get())
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
    
    
    
    using document_type = d3cpp::Document<Element>;
    
    // std::function<std::function<bool(Element*)> (const std::string&a)>
    auto tag_predicate = [](const std::string &tag) {
        return [tag](const Element* e) { return e->tag.compare(tag) == 0; };
    };

    std::function<ElementIterator(Element*)> gen_iter  = [](Element* e) { return ElementIterator(e); };

    {
        Element root("root");
        document_type document(&root);
        {
            std::vector<Point> points { {1,7}, {6,9}, {10,11} };
            
            auto selection = document
            .selectAll(tag_predicate("a"), gen_iter)
            .data(points); // join data into current selection
            
            selection
            .enter()
            .append([](Element* parent, const Point& p) { return &parent->append("a"); })
            .call([](Element *e, Point p) {
                e->attr("x",std::to_string(p.x));
                e->attr("y",std::to_string(p.y));
            });
            
            selection
            .call([](Element *e, Point p) {
                e->attr("new_attr","ABC");
            });
            
            std::cout << root;
        }
        
        
        {
            std::vector<Point> points { {29,30} };
            
            auto join_selection = document
            .selectAll(tag_predicate("a"), gen_iter)
            .data(points); // join data into current selection
            
            join_selection
            .exit()
            .remove([](Element *e) {
                // std::cerr << "removing " <<  *e << std::endl;
                e->remove();
            });
            
            join_selection
            .call([](Element *e, Point p) {
                e->attr("x_new",std::to_string(p.x));
                e->attr("y_new",std::to_string(p.y));
            });
            
            std::cout << root;
        }
        
        
        {   // forwarding data based on parent node's current data
            
            using list_type = std::vector<std::string>;
            
            std::vector<list_type> names { {"newton","eintstein","pythagoras"}, {"feyman","erdos"} };
            
            // append lists
            auto s1 = document
            .selectAll(tag_predicate("list"), gen_iter)
            .data(names);
            
            s1.enter()
            .append([](Element* parent, const list_type &list) { return &parent->append("list"); });
            
            // couldn't use an inline version of this function... why?
            // mapping
            
            using mapping_type = std::function<list_type(const list_type&)> ;
            
            //
            auto s2 = s1
            .selectAll(tag_predicate("name"), gen_iter)
            .data( (mapping_type) [](const list_type &s) { return s; });
            
            // .data(mapping);
            
            std::cout << s2 << std::endl;
            
            s2.enter()
            .append([](Element* parent, const std::string &st) { return &parent->append("name"); });
            
            s2
            .call([](Element* e, std::string s) { e->attr("str", s); });
            
            std::cout << root;
        }
    }

    
    
    { // mapping join

        std::cout << "##### data join using a mapping function ######" << std::endl;

        Element root("root");

        document_type document(&root);
        
        {
            std::vector<std::string> texts { "einstein", "newton", "pithagoras", "poincare" };
            
            // join
            auto selection = document
            .selectAll(tag_predicate("person"), gen_iter)
            .data(texts);
            
            // append elements
            selection
            .enter()
            .append([](Element* parent, const std::string& s) {
                return &parent->append("person");
            });
            
            // set name;
            selection
            .call([](Element* e, const std::string& s) {
                e->attr("name",s);
            });
        }
        
        std::cout << root << std::endl;
        
        // now is the time to join with a different set of strings
        {
            std::vector<std::string> update_texts { "einstein", "poincare", "feynman" };
        

            using s2s_type = std::function<std::string(const std::string&)> ;
            using e2s_type = std::function<std::string(const Element&)> ;
            
            
            s2s_type mapping_s = [](const std::string &s) -> std::string {
                return s;
            };

            e2s_type mapping_e = [](const Element &e) -> std::string {
                return e.attr("name");
            };
            
            // join
            auto selection = document
            .selectAll(tag_predicate("person"), gen_iter)
            .data(update_texts, mapping_s, mapping_e);
            
            //
            selection
            .exit()
            .remove([](Element *e) { e->remove(); });
            
            //
            selection
            .enter()
            .append([](Element* parent, const std::string& s) {
                return &parent->append("person");
            });

            //
            selection
            .call([](Element* e, const std::string& s) {
                e->attr("name",s);
            });
            
        }
        
        std::cout << root << std::endl;
        
        
        
        
        
        
        
        // how to join the current selection with the new
        // data coming in:
        
        // to avoid a O(n^2) search if we give just a "match(Element, Data)"
        // we can sort the elements "<"
        
        
        // assume Data element is sortable: map Element -> Data
        // match(Element,Data) lt(Element,Data) lt(Data,Data)
        
        // receive a vector of Data
        
        //
        // sort the input Data
        //
        // for each element do a binary search to find Data entry that matches
        //
        // assign that element to that data
        //
    
    }
    
    
    

    {
        std::cout << "##### forward data join and use of a mapping function ######" << std::endl;
        
        using list_type = std::vector<std::string>;
        
        Element root("root");
        
        document_type document(&root);
        
        {
            std::vector<list_type> names { {"einstein","gauss","feynman"}, {"pithagoras","newton"} };
            
            // append lists
            auto s1 = document
            .selectAll(tag_predicate("list"), gen_iter)
            .data(names);
            
            s1.enter()
            .append([](Element* parent, const list_type &list) { return &parent->append("list"); });
            
            // couldn't use an inline version of this function... why?
            // mapping
            
            using mapping_type = std::function<list_type(const list_type&)> ;
            
            //
            auto s2 = s1
            .selectAll(tag_predicate("name"), gen_iter)
            .data( (mapping_type) [](const list_type &s) { return s; });
            
            s2.enter()
            .append([](Element* parent, const std::string &st) { return &parent->append("name"); });
            
            s2
            .call([](Element* e, std::string s) { e->attr("str", s); });
            
            std::cout << root;
        }
        
        {
            std::vector<list_type> names { {"feynman","gauss","poincare"}, {"einstein","pithagoras"}, {"euclides", "newton"} };
            
            using s2s_type = std::function<std::string(const std::string&)> ;
            using e2s_type = std::function<std::string(const Element&)> ;
            
            s2s_type mapping_s = [](const std::string &s) -> std::string {
                return s;
            };
            
            e2s_type mapping_e = [](const Element &e) -> std::string {
                return e.attr("str");
            };

            // append lists
            auto s1 = document
            .selectAll(tag_predicate("list"), gen_iter)
            .data(names);
            
            s1
            .enter()
            .append([](Element* parent, const list_type &list) { return &parent->append("list"); });
            
            // couldn't use an inline version of this function... why?
            // mapping
            
            using mapping_type = std::function<list_type(const list_type&)> ;
            
            //
            auto s2 = s1
            .selectAll(tag_predicate("name"), gen_iter)
            .data( (mapping_type) [](const list_type &s) { return s; }, mapping_s, mapping_e);

            s2
            .exit()
            .remove([](Element *e) { e->remove(); });
            
            s2.enter()
            .append([](Element* parent, const std::string &st) { return &parent->append("name"); });
            
            s2
            .call([](Element* e, std::string s) { e->attr("str", s); });
            
            std::cout << root;
        }
        
        
    }
    
    
    
    
    
    
    
    
    

}
















