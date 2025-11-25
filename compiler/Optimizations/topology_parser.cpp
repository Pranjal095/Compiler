#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
using namespace std;

struct Edge {
    int u, v;
    double w;
};

bool parse_topology_file(const string& filename, int& n, int& m, vector<Edge>& edges) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "ERROR: Cannot open file: " << filename << endl;
        return false;
    }

    // Read first line: n m
    string first_line;
    if (!getline(file, first_line)) {
        cerr << "ERROR: Empty file or cannot read first line" << endl;
        return false;
    }

    stringstream ss(first_line);
    if (!(ss >> n >> m)) {
        cerr << "ERROR: First line must contain two integers (n m): " << first_line << endl;
        return false;
    }

    if (n <= 0 || m < 0) {
        cerr << "ERROR: Invalid values - n must be positive, m must be non-negative. Got n=" << n << ", m=" << m << endl;
        return false;
    }

    edges.clear();
    edges.reserve(m);

    // Read edges
    int line_num = 1;
    while (getline(file, first_line)) {
        line_num++;
        
        // Skip empty lines
        if (first_line.empty() || first_line.find_first_not_of(" \t\r\n") == string::npos) {
            continue;
        }

        // Replace commas with spaces for easier parsing
        for (char& c : first_line) {
            if (c == ',') c = ' ';
        }

        stringstream edge_ss(first_line);
        int u, v;
        double w = 1.0; // default weight

        if (!(edge_ss >> u >> v)) {
            cerr << "ERROR: Line " << line_num << " - Cannot parse edge (expected: u,v or u,v,w): " << first_line << endl;
            return false;
        }

        // Try to read weight (optional)
        edge_ss >> w;

        if (u < 0 || v < 0) {
            cerr << "ERROR: Line " << line_num << " - Vertex indices must be non-negative. Got u=" << u << ", v=" << v << endl;
            return false;
        }

        if (w <= 0.0) {
            cerr << "ERROR: Line " << line_num << " - Weight must be positive. Got w=" << w << endl;
            return false;
        }

        Edge e;
        e.u = u;
        e.v = v;
        e.w = w;
        edges.push_back(e);
    }

    if (edges.size() != (size_t)m) {
        cerr << "WARNING: Expected " << m << " edges but found " << edges.size() << " edges" << endl;
        m = edges.size(); // Update m to actual count
    }

    file.close();
    return true;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <topology_file.top>" << endl;
        return 1;
    }

    string filename = argv[1];
    int n, m;
    vector<Edge> edges;

    if (!parse_topology_file(filename, n, m, edges)) {
        cerr << "FAILED: Topology file validation failed for " << filename << endl;
        return 1;
    }

    cout << "SUCCESS: Topology file validated: " << filename << endl;
    cout << "  Vertices: " << n << endl;
    cout << "  Edges: " << m << endl;
    
    // Check for 0-based vs 1-based indexing
    bool has_zero = false, has_n = false;
    for (const auto& e : edges) {
        if (e.u == 0 || e.v == 0) has_zero = true;
        if (e.u == n || e.v == n) has_n = true;
    }
    
    if (!has_zero && has_n) {
        cout << "  Indexing: 1-based (detected)" << endl;
    } else if (has_zero && !has_n) {
        cout << "  Indexing: 0-based (detected)" << endl;
    } else {
        cout << "  Indexing: Mixed or ambiguous" << endl;
    }

    return 0;
}
