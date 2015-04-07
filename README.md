# d3cpp

Simple tests for simulating the d3 selection data join, update, enter, exit in C++

Example main:

```c++
int main() {
    
    struct Point {
        Point() = default;
        Point(int x, int y):
            x(x), y(y)
        {}
        int x;
        int y;
    };
    
    
    Element root("root");
    
    using document_type = d3cpp::Document<Element>;
    
    // std::function<std::function<bool(Element*)> (const std::string&a)>
    auto tag_predicate = [](const std::string &tag) {
        return [tag](const Element* e) { return e->tag.compare(tag) == 0; };
    };

    std::function<ElementIterator(Element*)> gen_iter  = [](Element* e) { return ElementIterator(e); };
    
    document_type document(&root);
    

    {
        std::vector<Point> points { {1,7}, {6,9}, {10,11} };

        auto selection = document
        .selectAll(tag_predicate("a"), gen_iter)
        .data(points); // join data into current selection
        
        selection
        .enter()
        .append([](Element* parent) { return &parent->append("a"); })
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

}
```
        
Output:

```xml
<root>
    <a new_attr="ABC" x="1" y="7"/>
    <a new_attr="ABC" x="6" y="9"/>
    <a new_attr="ABC" x="10" y="11"/>
</root>
<root>
    <a new_attr="ABC" x="1" x_new="29" y="7" y_new="30"/>
</root>
```
