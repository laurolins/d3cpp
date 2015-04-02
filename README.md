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
    
    std::vector<Point> points { {1,7}, {6,9}, {10,11} };
    
    Element root("root");
    
    Selection<int> sel(&root);

    auto update_sel = sel.data(points);
    
    update_sel.enter().append("a")
       .attr("x",[](const Point& p, int i) { return std::to_string(p.x); })
       .attr("y",[](const Point& p, int i) { return std::to_string(p.y); });

    update_sel
    .attr("x",[](const Point& p, int i) { return std::to_string(p.y); })
    .attr("y",[](const Point& p, int i) { return std::to_string(p.x); });

    std::cout << root;
    
}
```
        
Output:

    <root>
        <a x="7" y="1"/>
        <a x="9" y="6"/>
        <a x="11" y="10"/>
    </root>
