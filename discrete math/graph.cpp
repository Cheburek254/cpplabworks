#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <climits>
#include <set>
#include <string>

using namespace std;


//ребро графа для списка смежности
struct Edge {
    int to;
    int weight;
    Edge(int t, int w) : to(t), weight(w) {}
};

//ребро графа
struct FullEdge {
    int u, v, weight;
    FullEdge(int from, int to, int w) : u(from), v(to), weight(w) {}
    bool operator<(const FullEdge& other) const {
        return weight < other.weight;
    }
};

//взыешанный граф
class G{
private:
    int n; //|E|
    vector<vector<Edge>> graph; //список смежности
    vector<FullEdge> edges; // список ребер


    //алгоритм дейкстры
    vector<int> Dijkstra(int start) {
        vector<int> dist(n, INT_MAX);
        dist[start] = 0;
        priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int, int>>> pq;
        pq.push({0, start});

        while (!pq.empty()) {
            int d = pq.top().first;
            int u = pq.top().second;
            pq.pop();

            if (d > dist[u]) continue;

            for (const Edge& e : graph[u]) {
                // Используем вес 1 для подсчета количества ребер
                if (dist[u] != INT_MAX && dist[u] + 1 < dist[e.to]) {
                    dist[e.to] = dist[u] + 1;
                    pq.push({dist[e.to], e.to});
                }
            }
        }
        return dist;
    }
    //жаднвая покраска бфсом
    int bfsColoring(vector<int>& color) {
        queue<int> q;
        q.push(0);
        color[0] = 0; 
        
        int maxColor = 0;
        
        while (!q.empty()) {
            int current = q.front();
            q.pop();

            for (const Edge& e : graph[current]) {
                int neighbor = e.to;
                
                if (color[neighbor] == -1) {
                    vector<bool> usedColors(n + 1, false);
                    for (const Edge& ne : graph[neighbor]) {
                        if (color[ne.to] != -1) {
                            usedColors[color[ne.to]] = true;
                        }
                    }
                    
                    int newColor = 0;
                    while (usedColors[newColor]) {
                        newColor++;
                    }
                    
                    color[neighbor] = newColor;
                    maxColor = max(maxColor, newColor);
                    q.push(neighbor);
                }
            }
        }
        
        return maxColor + 1; 
    }
    //бинарное кодирование дфсом(поход налево, вниз 1 вверх 0)
    void dfsbinary(int u, int parent, const vector<vector<int>>& mstgraph, string& binaryCode) {
        for (int v : mstgraph[u]) {
            if (v != parent) {
                binaryCode += '1';
                dfsbinary(v, u, mstgraph, binaryCode);
                binaryCode += '0';
            }
        }
    }
    string getCountryName(int c) {
    switch(c) {
        case 0: return "Por";
        case 1: return "Spa";
        case 2: return "Fra";
        case 3: return "Bel";
        case 4: return "Net";
        case 5: return "Lux";
        case 6: return "Ger";
        case 7: return "Aus";
        case 8: return "Ita";
        case 9: return "Slovenia";
        case 10: return "Cro";
        case 11: return "Hun";
        case 12: return "Rom";
        case 13: return "Bul";
        case 14: return "Gre";
        case 15: return "Slo";
        case 16: return "Cze";
        case 17: return "Pol";
        case 18: return "Den";
        case 19: return "Lit";
        case 20: return "Lat";
        case 21: return "Est";
        default: return "Unknown";
    }
}


public:
    G(int count): n(count){
        graph.resize(n);
    }

    void AddEdge(int u, int v, int w){
        graph[u].emplace_back(v,w);
        graph[v].emplace_back(u,w);
        edges.emplace_back(u,v,w);
    }
    
    //поиск радиуса диаметра центра
    void FindRDC(){
    vector<int> excentrisitet(n, 0);
    
    // Проверяем связность графа
    vector<int> dist0 = Dijkstra(0);
    bool isConnected = true;
    for (int i = 0; i < n; i++) {
        if (dist0[i] == INT_MAX) {
            isConnected = false;
            break;
        }
    }
    
    if (!isConnected) {
        cout << "Graph not full connected" << endl;
        // Здесь можно реализовать поиск компонент связности
        return;
    }
    
    //находим все эксцентриситеты через дейкстру
    for (int i = 0; i < n; i++) {
        vector<int> dist = Dijkstra(i);
        int maxDist = 0;
        for (int j = 0; j < n; j++) {
            if (i != j && dist[j] != INT_MAX) {
                maxDist = max(maxDist, dist[j]);
            }
        }
        excentrisitet[i] = maxDist;
    }

    int radius = INT_MAX;
    int diameter = 0;
    for (int e : excentrisitet) {
        if (e > 0) { // Игнорируем вершины с нулевым эксцентриситетом (изолированные)
            radius = min(radius, e);
            diameter = max(diameter, e);
        }
    }
    
    vector<int> centers;
    for (int i = 0; i < n; i++) {
        if (excentrisitet[i] == radius) {
            centers.push_back(i);
        }
    }

    cout << "Radius : " << radius << endl;
    cout << "Diameter : " << diameter << endl;
    cout << "Center :" << endl;
    for (int c : centers) {
        cout << "  Verticie " << c ; 
        cout << getCountryName(c);
        cout << endl;
    }
    cout << endl;
}

    int ChromaticNumber(){
        vector<int> color(n, -1);
        int chromaticNumber = bfsColoring(color);
        cout << "Chromatic number " << chromaticNumber << endl;
        return chromaticNumber;
    }
        
    //Система непересекающихся множест для быстрого построения MST
    class DSU{
    private:
        vector<int> parent, rank;
    public:
        DSU(int n){
            parent.resize(n);
            rank.resize(n);
            for (int i = 0; i < n; ++i) parent[i] = i;
        }
        int find(int x){
            if(parent[x] != x) parent[x] = find(parent[x]);
            return parent[x];
        }
        void unite(int x, int y) {
            int rootX = find(x);
            int rootY = find(y);
            if (rootX != rootY) {
                if (rank[rootX] < rank[rootY]) parent[rootX] = rootY;
                else if (rank[rootX] > rank[rootY]) parent[rootY] = rootX;
                else {
                    parent[rootY] = rootX;
                    rank[rootX]++;
                }
            }
        }
    };
    //поиск mst
    vector<FullEdge> findMST() {
        vector<FullEdge> sortedEdges = edges;
        sort(sortedEdges.begin(), sortedEdges.end());

        DSU dsu(n);
        vector<FullEdge> mst;
        int totalWeight = 0;

        for (const FullEdge& e : sortedEdges) {
            if (dsu.find(e.u) != dsu.find(e.v)) {
                dsu.unite(e.u, e.v);
                mst.push_back(e);
                totalWeight += e.weight;
            }
        }

        cout << "MST edges:\n";
        for (const FullEdge& e : mst) {
            cout << "      " << getCountryName(e.u) << " - " << getCountryName(e.v) << " (weight: " << e.weight << ")\n";
        }
        cout << endl;
        return mst;
    }

    //кодирование дерефа прюфером
    string getPrueferCode(const vector<FullEdge>& mst) {
        int m = n;
        vector<vector<int>> mstAdj(m);
        for (const FullEdge& e : mst) {
            mstAdj[e.u].push_back(e.v);
            mstAdj[e.v].push_back(e.u);
        }
        vector<int> degree(m, 0);
        for (int i = 0; i < m; i++) {
            degree[i] = mstAdj[i].size();
        }
        vector<int> pruefer;
        set<int> leaves;
        for (int i = 0; i < m; i++) {
            if (degree[i] == 1) {
                leaves.insert(i);
            }
        }
        for (int i = 0; i < m - 2; i++) {
            int leaf = *leaves.begin();
            leaves.erase(leaves.begin());
            int neighbor = -1;
            for (int v : mstAdj[leaf]) {
                if (degree[v] > 0) {
                    neighbor = v;
                    break;
                }
            }
            pruefer.push_back(neighbor);
            degree[leaf]--;
            degree[neighbor]--;
            if (degree[neighbor] == 1) {
                leaves.insert(neighbor);
            }
        }

        string result;
        for (int code : pruefer) {
            result += to_string(code ) + '|';
        }
        return result;
    }

    //кодирование дерева бинарным кодом через дфс
    string getBinaryCode(const vector<FullEdge>& mst) {
        vector<vector<int>> mstgraph(n);
        for (const FullEdge& e : mst) {
            mstgraph[e.u].push_back(e.v);
            mstgraph[e.v].push_back(e.u);
        }
        
        for (int i = 0; i < n; i++) {
            sort(mstgraph[i].begin(), mstgraph[i].end());
        }
        
        string binaryCode;
        dfsbinary(0, -1, mstgraph, binaryCode);
        
        return binaryCode;
    }
    void ShowEncodedMST() {
        vector<FullEdge> mst = findMST();
        
        string pruefer = getPrueferCode(mst);
        string binary = getBinaryCode(mst);
        
       
        cout << "   Preufer code: " << pruefer << "\n";
        cout << "   Binary code: " << binary << "\n";
        
    }
};

int main() {
    G g(22);

    /*
    Вершины такие как Ирландия, Швеция - Финляндия, Мальта, Кипр при решении задач учитваться не будут, так как они не влияют на ответы в данном задании,
    так как мы рассматриваем максимальную компоненту связности.
    В поиске хроматического числа гарантируется что оно будет меньше 4, так как граф планарный, что можно увидеть по рисунку
    Название страны   ее номер
    Португалия         0
    Испания            1
    Франция            2
    Бельгия            3
    Нидерланды         4
    Люксембург         5
    Германия           6
    Австрия            7
    Италия             8
    Словения           9
    Хорватия           10
    Венгрия            11
    Румыния            12
    Болгария           13
    Греция             14
    Словакия           15
    Чехия              16
    Польша             17
    ДАния              18
    Литва              19
    Латвия             20
    Эстония            21
    */

    
    g.AddEdge(0, 1, 500);
    g.AddEdge(1, 2, 1000);
    g.AddEdge(2, 3, 250);
    g.AddEdge(2, 5, 300);
    g.AddEdge(2, 6, 900);
    g.AddEdge(2, 8, 1100);
    g.AddEdge(3, 4, 150);
    g.AddEdge(3, 5, 200);
    g.AddEdge(3, 6, 650);
    g.AddEdge(4, 6, 550);
    g.AddEdge(5, 6, 600);
    g.AddEdge(6, 18, 350);
    g.AddEdge(6, 17, 500);
    g.AddEdge(6, 16, 300);
    g.AddEdge(6, 7, 500);
    g.AddEdge(7, 15, 55);
    g.AddEdge(7, 11, 215);
    g.AddEdge(7, 9, 250);
    g.AddEdge(7, 8, 750);
    g.AddEdge(8, 9, 500);
    g.AddEdge(9, 10, 100);
    g.AddEdge(9, 11, 400);
    g.AddEdge(10, 11, 300);
    g.AddEdge(11, 12, 650);
    g.AddEdge(11, 15, 150);
    g.AddEdge(12, 13, 300);
    g.AddEdge(13, 14, 500);
    g.AddEdge(15, 16, 300);
    g.AddEdge(15, 17, 550);
    g.AddEdge(16, 17, 500);
    g.AddEdge(17, 19, 400);
    g.AddEdge(19, 20, 250);
    g.AddEdge(20, 21, 300);

    

     cout << "Analisy of graph" << endl;
    g.FindRDC();
    cout << endl;
    
    cout << "Chromatic number" << endl;
    g.ChromaticNumber();
    cout << endl;
    
    cout << "MST" << endl;
    g.ShowEncodedMST();

    
    return 0;
}
