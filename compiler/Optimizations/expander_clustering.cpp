
#include <bits/stdc++.h>
#include <mpi.h>
#include <fstream>
using namespace std;

struct Timer {
    chrono::high_resolution_clock::time_point t0;
    void start(){ t0=chrono::high_resolution_clock::now(); }
    double stop_ms() const {
        return chrono::duration<double, std::milli>(chrono::high_resolution_clock::now()-t0).count();
    }
};

// weighted graph
struct Graph {
    int n;
    vector<vector<pair<int,double>>> adj;
    Graph(int n=0): n(n), adj(n) {}
    void add_edge(int u,int v,double w){
        if(u==v) return;
        adj[u].emplace_back(v,w);
        adj[v].emplace_back(u,w);
    }
};

// simple edge struct for MPI broadcast
struct Edge {
    int u, v;
    double w;
};

// params & helpers 
static const int MAX_ITERS = 300;
static const int MIN_ITERS = 30;
static const double TOL = 1e-8;
static const int KMEANS_ITERS = 200;

// PPR params (tunable)
static const double PPR_ALPHA = 0.15;
static const double PPR_EPS   = 1e-4;

static inline bool is_one_based_with_edges(const vector<Edge>& edges, int n){
    bool has_zero=false, has_n=false;
    for (auto &e: edges){
        int a = e.u, b = e.v;
        if (a==0 || b==0) has_zero=true;
        if (a==n || b==n) has_n=true;
    }
    return (!has_zero && has_n);
}

static double dot(const vector<double>& a, const vector<double>& b){
    long double s=0;
    for(size_t i=0;i<a.size();++i) s += (long double)a[i]*b[i];
    return (double)s;
}
static double norm2(const vector<double>& a){
    return sqrt(max(1e-300, dot(a,a)));
}
static void normalize(vector<double>& a){
    double n = norm2(a);
    if(n<1e-300) return;
    for(double &x: a) x/=n;
}
static void axpy(vector<double>& y, double alpha, const vector<double>& x){
    for(size_t i=0;i<y.size();++i) y[i] += alpha * x[i];
}
static void proj_orth(vector<double>& v, const vector<vector<double>>& basis){
    for (const auto &b: basis){
        double c = dot(v,b);
        axpy(v, -c, b);
    }
}

// induced utilities 
static vector<int> induced_map(const vector<int>& nodes, int N){
    vector<int> in(N, -1);
    for (int i=0;i<(int)nodes.size(); ++i) in[nodes[i]] = i;
    return in;
}
static vector<double> induced_weighted_degrees(const Graph& G, const vector<int>& nodes, const vector<int>& in_sub) {
    int m = (int)nodes.size();
    vector<double> deg(m, 0.0);
    for (int i=0;i<m;++i){
        int u = nodes[i];
        for (auto &pr: G.adj[u]){
            int v = pr.first; double w = pr.second;
            if (in_sub[v] != -1) deg[i] += w;
        }
    }
    return deg;
}

// global weighted degrees
static vector<double> global_weighted_degrees(const Graph& G){
    vector<double> deg(G.n, 0.0);
    for (int u=0; u<G.n; ++u){
        for (auto &pr: G.adj[u]) deg[u] += pr.second;
    }
    return deg;
}

// S * x 
static void S_times_vec(const Graph& G, const vector<int>& nodes, const vector<int>& in_sub, const vector<double>& degH, const vector<double>& x, vector<double>& y) {
    int m = (int)nodes.size();
    vector<double> z(m, 0.0);
    for (int i=0;i<m;++i) z[i] = x[i] / sqrt(max(1e-300, degH[i]));
    fill(y.begin(), y.end(), 0.0);
    for (int i=0;i<m;++i){
        int u = nodes[i];
        for (auto &pr: G.adj[u]){
            int v = pr.first; double w = pr.second;
            int j = in_sub[v];
            if (j != -1) y[i] += w * z[j];
        }
    }
    for (int i=0;i<m;++i) y[i] = y[i] / sqrt(max(1e-300, degH[i]));
}

// eigenvectors (power iteration + deflation) 
static vector<vector<double>> top_k_eigenvectors_S(const Graph& G, const vector<int>& nodes, int k, int seed=42){
    int m = (int)nodes.size();
    vector<int> in_sub = induced_map(nodes, G.n);
    vector<double> degH = induced_weighted_degrees(G, nodes, in_sub);

    vector<vector<double>> eigs; eigs.reserve(k);
    mt19937 rng(seed);
    normal_distribution<double> ND(0.0,1.0);

    for (int t=0; t<k; ++t){
        vector<double> v(m);
        for (int i=0;i<m;++i) v[i] = ND(rng);
        proj_orth(v, eigs);
        normalize(v);

        vector<double> y(m,0.0);
        double prev_cos = 0.0;
        for (int it=0; it<MAX_ITERS; ++it){
            S_times_vec(G, nodes, in_sub, degH, v, y);
            proj_orth(y, eigs);
            double nrm = norm2(y);
            if (nrm < 1e-16){
                for (int i=0;i<m;++i) y[i] = ND(rng);
                proj_orth(y, eigs);
                nrm = norm2(y);
                if (nrm < 1e-16) break;
            }
            for (int i=0;i<m;++i) y[i] /= nrm;
            double c = fabs(dot(v,y));
            v.swap(y);
            if (it >= MIN_ITERS && fabs(c - prev_cos) < TOL) break;
            prev_cos = c;
        }
        eigs.push_back(v);
    }
    return eigs;
}

// sweep for weighted conductance 
struct CutResult { vector<int> S_indices; double conductance; };

static CutResult sweep_min_conductance(const Graph& G, const vector<int>& nodes, const vector<double>& key){
    int m = (int)nodes.size();
    vector<int> order(m); iota(order.begin(), order.end(), 0);
    sort(order.begin(), order.end(), [&](int a, int b){
        if (key[a] != key[b]) return key[a] < key[b];
        return a < b;
    });
    vector<int> in_sub = induced_map(nodes, G.n);
    vector<double> degH = induced_weighted_degrees(G, nodes, in_sub);
    double volH = 0.0; for (double d: degH) volH += d;
    vector<char> inS(m, 0);
    double volS = 0.0, cut = 0.0;
    double best_phi = 1e100; int best_t = -1;
    for (int t=0; t<m-1; ++t){
        int i = order[t];
        int u = nodes[i];
        for (auto &pr: G.adj[u]){
            int v = pr.first; double w = pr.second;
            int j = in_sub[v];
            if (j == -1) continue;
            if (inS[j]) cut -= w; else cut += w;
        }
        inS[i] = 1;
        volS += degH[i];
        double denom = min(volS, volH - volS);
        if (denom > 0){
            double phi = cut / denom;
            if (phi < best_phi){ best_phi = phi; best_t = t; }
        }
    }
    CutResult R; R.conductance = (best_t == -1 ? 1e100 : best_phi);
    if (best_t != -1){
        R.S_indices.reserve(best_t+1);
        for (int t=0;t<=best_t;++t) R.S_indices.push_back(order[t]);
    }
    return R;
}

// spectral split + merge helpers (used for refinement/merging) 
static bool try_spectral_split_best_weighted(const Graph& G, const vector<int>& cluster_nodes, vector<int>& left_nodes, vector<int>& right_nodes, double min_cluster_size){
    if ((int)cluster_nodes.size() < 2 * max(1, (int)min_cluster_size)) return false;
    auto eigs = top_k_eigenvectors_S(G, cluster_nodes, 2, 123456);
    if (eigs.size() < 2) return false;
    const vector<double>& f = eigs[1];
    auto cut = sweep_min_conductance(G, cluster_nodes, f);
    if (cut.S_indices.empty()) return false;
    vector<char> inS(cluster_nodes.size(), 0);
    for (int idx: cut.S_indices) inS[idx] = 1;
    vector<int> L, R;
    for (int i=0;i<(int)cluster_nodes.size();++i){
        if (inS[i]) L.push_back(cluster_nodes[i]); else R.push_back(cluster_nodes[i]);
    }
    if ((int)L.size() == 0 || (int)R.size() == 0) return false;
    if ((int)L.size() < (int)min_cluster_size || (int)R.size() < (int)min_cluster_size) return false;
    left_nodes = move(L); right_nodes = move(R);
    return true;
}

static void merge_small_with_best_neighbor_weighted(const Graph& G, vector<vector<int>>& clusters){
    int k = clusters.size();
    if (k <= 1) return;
    vector<int> part(G.n, -1);
    for (int i=0;i<k;++i) for (int u: clusters[i]) part[u] = i;
    vector<vector<double>> pairw(k, vector<double>(k, 0.0));
    for (int u=0; u<G.n; ++u){
        for (auto &pr: G.adj[u]){
            int v = pr.first; double w = pr.second;
            if (u < v){
                int a = part[u], b = part[v];
                if (a != b && a != -1 && b != -1) {
                    pairw[a][b] += w; pairw[b][a] += w;
                }
            }
        }
    }
    int smallest = -1; int minsz = INT_MAX;
    for (int i=0;i<k;++i)
        if ((int)clusters[i].size() < minsz){ minsz = (int)clusters[i].size(); smallest = i; }
    if (smallest == -1) return;
    int best_nb = -1; double bestw = -1.0;
    for (int j=0;j<k;++j) if (j != smallest){
        if (pairw[smallest][j] > bestw){ bestw = pairw[smallest][j]; best_nb = j; }
    }
    if (best_nb == -1) best_nb = (smallest == 0 ? 1 : 0);
    for (int u: clusters[smallest]) clusters[best_nb].push_back(u);
    clusters.erase(clusters.begin() + smallest);
}

// metrics 
static double count_cut_weighted_edges(const Graph& G, const vector<int>& C, const vector<char>& inC){
    double total = 0.0;
    for (int u: C){
        for (auto &pr: G.adj[u]){
            int v = pr.first; double w = pr.second;
            if (!inC[v]) total += w;
        }
    }
    return total;
}
static double external_conductance_weighted(const Graph& G, const vector<int>& C){
    vector<char> inC(G.n, 0);
    for (int u: C) inC[u] = 1;
    double volC = 0.0, volRest = 0.0;
    for (int u=0; u<G.n; ++u){
        if (inC[u]) for (auto &pr: G.adj[u]) volC += pr.second;
        else for (auto &pr: G.adj[u]) volRest += pr.second;
    }
    double cut = count_cut_weighted_edges(G, C, inC);
    double denom = min(volC, volRest);
    if (denom <= 0.0) return 0.0;
    return cut / denom;
}
static double spectral_gap_proxy_weighted(const Graph& G, const vector<int>& C){
    if ((int)C.size() <= 1) return 0.0;
    auto eigs = top_k_eigenvectors_S(G, C, 2, 9001);
    if (eigs.size() < 2) return 0.0;
    vector<int> in_sub = induced_map(C, G.n);
    vector<double> degH = induced_weighted_degrees(G, C, in_sub);
    vector<double> Sv(C.size(), 0.0);
    S_times_vec(G, C, in_sub, degH, eigs[1], Sv);
    double mu2 = dot(eigs[1], Sv);
    double gap = 1.0 - mu2;
    if (gap < 0) gap = 0;
    if (gap > 2) gap = 2;
    return gap;
}

struct AlgoMetrics {
    double runtime_ms=0.0;
    double cut_edges=0.0;
    double total_edges=0.0;
    vector<int> sizes;
    vector<double> ext_conductance;
    vector<double> gap1_minus_mu2;
};

static AlgoMetrics evaluate_partition_weighted(const Graph& G, const vector<vector<int>>& clusters, double runtime_ms){
    AlgoMetrics M; M.runtime_ms = runtime_ms;
    double m_total = 0.0;
    for (int u=0; u<G.n; ++u) for (auto &pr: G.adj[u]) m_total += pr.second;
    m_total /= 2.0;
    M.total_edges = m_total;

    int k = (int)clusters.size();
    double cut_total = 0.0;
    for (int i=0;i<k;++i){
        const auto &C = clusters[i];
        vector<char> inC(G.n, 0); for (int u: C) inC[u] = 1;
        double cut = count_cut_weighted_edges(G, C, inC);
        cut_total += cut;
        M.sizes.push_back((int)C.size());
        M.ext_conductance.push_back(external_conductance_weighted(G, C));
        M.gap1_minus_mu2.push_back(spectral_gap_proxy_weighted(G, C));
    }
    M.cut_edges = cut_total / 2.0;
    return M;
}

static void write_metrics_to_file(const AlgoMetrics& M, const string& stats_file){
    ofstream out(stats_file);
    if (!out.is_open()) {
        cerr << "ERROR: Cannot write to stats file: " << stats_file << endl;
        return;
    }
    
    out << "Runtime (ms): " << fixed << setprecision(2) << M.runtime_ms << "\n";
    out << "Total weighted edges: " << fixed << setprecision(4) << M.total_edges << "\n";
    out << "Weighted cut edges: " << M.cut_edges << "\n";
    out << "Cut percentage: " << fixed << setprecision(2) << (100.0 * (double)M.cut_edges / max(1e-12, M.total_edges)) << "%\n";
    
    int k = (int)M.sizes.size();
    int minsz = INT_MAX, maxsz = 0; long long sumsz = 0;
    double mean_phi = 0.0, min_phi = 1e300, max_phi = 0.0;
    double mean_gap = 0.0, min_gap = 1e300, max_gap = 0.0;
    
    for (int i=0;i<k;++i){
        minsz = min(minsz, M.sizes[i]); maxsz = max(maxsz, M.sizes[i]); sumsz += M.sizes[i];
        mean_phi += M.ext_conductance[i]; min_phi = min(min_phi, M.ext_conductance[i]); max_phi = max(max_phi, M.ext_conductance[i]);
        mean_gap += M.gap1_minus_mu2[i]; min_gap = min(min_gap, M.gap1_minus_mu2[i]); max_gap = max(max_gap, M.gap1_minus_mu2[i]);
    }
    
    out << "\nCluster Statistics:\n";
    out << "  Number of clusters: " << k << "\n";
    out << "  Cluster sizes - min: " << minsz << ", max: " << maxsz << ", mean: " << fixed << setprecision(2) << ((double)sumsz / k) << "\n";
    out << "  External conductance - min: " << min_phi << ", max: " << max_phi << ", mean: " << (mean_phi / k) << "\n";
    out << "  Internal expansion (1-μ2) - min: " << min_gap << ", max: " << max_gap << ", mean: " << (mean_gap / k) << "\n";
    
    out << "\nPer-Cluster Details:\n";
    for (int i=0; i<k; ++i) {
        out << "Cluster " << (i+1) << ": size=" << M.sizes[i] << ", conductance=" << fixed << setprecision(4) << M.ext_conductance[i] << ", expansion=" << M.gap1_minus_mu2[i] << "\n";
    }
    
    out.close();
}


// local PPR or heat-kernel style clustering 
// Push-based approximate PPR from a single seed
static void local_push_ppr(const Graph& G, const vector<double>& deg, int seed, double alpha, double eps, vector<double>& p_out) {
    int n = G.n;
    vector<double> p(n, 0.0), r(n, 0.0);
    r[seed] = 1.0;
    queue<int> q;
    vector<char> inq(n, 0);
    q.push(seed); inq[seed] = 1;

    while (!q.empty()){
        int u = q.front(); q.pop(); inq[u] = 0;
        double du = max(deg[u], 1e-12);
        if (r[u] / du <= eps) continue;

        double ru = r[u];
        r[u] = 0.0;

        // classic ACL-style push
        p[u] += alpha * ru;
        double push_mass = (1.0 - alpha) * ru;

        double share = push_mass / (2.0 * du); // half stays, half goes to neighbors
        r[u] += share * du;

        for (auto &pr : G.adj[u]){
            int v = pr.first; double w = pr.second;
            double delta = share * w;
            r[v] += delta;
            if (!inq[v] && r[v]/max(deg[v],1e-12) > eps){
                q.push(v);
                inq[v] = 1;
            }
        }
    }
    p_out.swap(p);
}

// PPR-based cluster extraction by sweep over p(v)/deg(v)
static void ppr_seed_cluster(const Graph& G, const vector<double>& deg, int seed, vector<int>& cluster_out) {
    int n = G.n;
    vector<double> p;
    local_push_ppr(G, deg, seed, PPR_ALPHA, PPR_EPS, p);

    vector<int> nodes(n);
    iota(nodes.begin(), nodes.end(), 0);

    vector<double> score(n);
    for (int i=0;i<n;++i){
        double di = max(deg[i], 1e-12);
        score[i] = p[i] / di; // standard sweep score
    }

    auto cut = sweep_min_conductance(G, nodes, score);
    if (cut.S_indices.empty()){
        // fallback: singleton cluster
        cluster_out.clear();
        cluster_out.push_back(seed);
        return;
    }

    vector<char> inS(n,0);
    for (int idx: cut.S_indices){
        int v = nodes[idx];
        inS[v] = 1;
    }

    // ensure seed is inside; if not, complement
    if (!inS[seed]){
        for (int i=0;i<n;++i) inS[i] = !inS[i];
    }

    cluster_out.clear();
    for (int v=0; v<n; ++v){
        if (inS[v]) cluster_out.push_back(v);
    }
    if (cluster_out.empty()){
        cluster_out.push_back(seed);
    }
}

// distributed Algo using MPI and local PPR 
static bool algo1_best_effort_weighted_mpi(const Graph& G, int k, vector<vector<int>>& clusters_out, int rank, int size) {
    int n = G.n;
    if (k > n) return false;

    // Precompute global degrees (replicated on all ranks)
    auto deg = global_weighted_degrees(G);

    // choose seeds on rank 0 
    vector<int> seeds;
    if (rank == 0){
        // simple heuristic: pick high-degree vertices as seeds
        vector<int> order(n);
        iota(order.begin(), order.end(), 0);
        sort(order.begin(), order.end(), [&](int a,int b){
            if (deg[a] != deg[b]) return deg[a] > deg[b];
            return a < b;
        });

        int desired = max(k * 3, size * 2); // redundantly many seeds
        desired = min(desired, n);
        seeds.reserve(desired);
        for (int i=0;i<desired;++i) seeds.push_back(order[i]);
    }

    // broadcast seeds
    int num_seeds = (int)seeds.size();
    MPI_Bcast(&num_seeds, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if (rank != 0) seeds.resize(num_seeds);
    if (num_seeds > 0)
        MPI_Bcast(seeds.data(), num_seeds, MPI_INT, 0, MPI_COMM_WORLD);

    // distribute seeds among ranks 
    vector<int> local_seeds;
    for (int i=0;i<num_seeds;++i){
        if (i % size == rank) local_seeds.push_back(seeds[i]);
    }

    // each rank runs local PPR clustering for its seeds 
    vector<vector<int>> local_clusters;
    for (int s : local_seeds){
        vector<int> C;
        ppr_seed_cluster(G, deg, s, C);
        sort(C.begin(), C.end());
        C.erase(unique(C.begin(), C.end()), C.end());
        local_clusters.push_back(move(C));
    }

    // serialize local clusters for gather 
    vector<int> sendbuf;
    sendbuf.push_back((int)local_clusters.size());
    for (auto &C : local_clusters){
        sendbuf.push_back((int)C.size());
        for (int v : C) sendbuf.push_back(v);
    }
    int local_len = (int)sendbuf.size();

    vector<int> recv_counts;
    if (rank == 0) recv_counts.resize(size);

    MPI_Gather(&local_len, 1, MPI_INT, rank==0 ? recv_counts.data() : nullptr, 1, MPI_INT, 0, MPI_COMM_WORLD);

    vector<int> recvbuf, displs;
    if (rank == 0){
        displs.resize(size);
        int total = 0;
        for (int i=0;i<size;++i){
            displs[i] = total;
            total += recv_counts[i];
        }
        recvbuf.resize(total);
    }

    MPI_Gatherv(sendbuf.data(), local_len, MPI_INT, rank==0 ? recvbuf.data() : nullptr, rank==0 ? recv_counts.data() : nullptr, rank==0 ? displs.data() : nullptr, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank != 0){
        // non-root nodes done with clustering; root will produce final partition
        return true;
    }

    // root: reconstruct clusters from all ranks 
    vector<vector<int>> all_clusters;
    for (int p=0; p<size; ++p){
        int len = recv_counts[p];
        if (len == 0) continue;
        int pos = displs[p];
        int num_loc = recvbuf[pos++];
        for (int i=0;i<num_loc;++i){
            int sz = recvbuf[pos++];
            vector<int> C(sz);
            for (int j=0;j<sz;++j) C[j] = recvbuf[pos++];
            all_clusters.push_back(move(C));
        }
    }

    if (all_clusters.empty()){
        // fallback: trivial partition into k contiguous chunks
        clusters_out.clear();
        clusters_out.resize(k);
        for (int v=0; v<n; ++v) clusters_out[v % k].push_back(v);
        return true;
    }

    // boundary negotiation: create a proper partition 
    // Build vertex and best cluster assignment (tie-break by cluster size)
    vector<int> best_cluster_for_v(n, -1);
    vector<int> cluster_size;
    cluster_size.reserve(all_clusters.size());
    for (auto &C : all_clusters) cluster_size.push_back((int)C.size());

    for (int cid=0; cid<(int)all_clusters.size(); ++cid){
        for (int v: all_clusters[cid]){
            if (best_cluster_for_v[v] == -1 ||
                cluster_size[cid] > cluster_size[best_cluster_for_v[v]]){
                best_cluster_for_v[v] = cid;
            }
        }
    }

    // assign unassigned vertices greedily to neighbor cluster or random
    std::mt19937 rng(123456);
    for (int v=0; v<n; ++v){
        if (best_cluster_for_v[v] != -1) continue;
        int best_c = -1;
        double best_w = -1.0;
        for (auto &pr : G.adj[v]){
            int u = pr.first; double w = pr.second;
            int c = best_cluster_for_v[u];
            if (c != -1 && w > best_w){
                best_w = w; best_c = c;
            }
        }
        if (best_c == -1) best_c = rng() % (int)all_clusters.size();
        best_cluster_for_v[v] = best_c;
    }

    // build partition clusters from assignments
    int Cmax = 1 + *max_element(best_cluster_for_v.begin(), best_cluster_for_v.end());
    vector<vector<int>> clusters(Cmax);
    for (int v=0; v<n; ++v){
        int c = best_cluster_for_v[v];
        if (c < 0) c = 0;
        clusters[c].push_back(v);
    }

    // drop empty clusters
    {
        vector<vector<int>> tmp;
        for (auto &C : clusters) if (!C.empty()) tmp.push_back(move(C));
        clusters.swap(tmp);
    }

    if ((int)clusters.size() == 0){
        clusters_out.clear();
        clusters_out.resize(k);
        for (int v=0; v<n; ++v) clusters_out[v % k].push_back(v);
        return true;
    }

    // greedy merging to reach at most k clusters 
    while ((int)clusters.size() > k){
        merge_small_with_best_neighbor_weighted(G, clusters);
    }

    // if we have fewer than k clusters, try splitting large ones (spectral refinement) 
    int min_cluster_relaxed = 1;
    while ((int)clusters.size() < k){
        int idx = -1, mx = -1;
        for (int i=0;i<(int)clusters.size(); ++i)
            if ((int)clusters[i].size() > mx){ mx = (int)clusters[i].size(); idx = i; }
        if (idx == -1 || mx < 2) break;
        vector<int> L, R;
        if (!try_spectral_split_best_weighted(G, clusters[idx], L, R, min_cluster_relaxed)) break;
        clusters[idx] = move(L);
        clusters.push_back(move(R));
    }

    // optional local spectral refinement: one pass, split clusters only if improves conductance 
    {
        vector<vector<int>> refined;
        for (auto &C : clusters){
            double phi_before = external_conductance_weighted(G, C);
            vector<int> L,R;
            bool ok = try_spectral_split_best_weighted(G, C, L, R, max(1, (int)C.size()/4));
            if (!ok){
                refined.push_back(move(C));
                continue;
            }
            double phi_L = external_conductance_weighted(G, L);
            double phi_R = external_conductance_weighted(G, R);
            double best_phi = min(phi_L, phi_R);
            if (best_phi + 1e-6 < phi_before && (int)refined.size()+1 < k*2){
                refined.push_back(move(L));
                refined.push_back(move(R));
            } else {
                refined.push_back(move(C));
            }
        }
        clusters.swap(refined);
    }

    // final merge to exactly k
    while ((int)clusters.size() > k){
        merge_small_with_best_neighbor_weighted(G, clusters);
    }

    clusters_out = move(clusters);
    return true;
}

// Determine optimal k using quality metric
// Uses ceil(sqrt(n)) as upper bound based on expander graph theory:
// Expander mixing lemma suggests ~sqrt(n) is the natural limit for well-balanced clusters
// Beyond sqrt(n), clusters become too small to maintain good internal expansion
static int find_optimal_k(const Graph& G, int rank, int size) {
    int n = G.n;
    if (n <= 2) return 1;
    
    // Always test up to ceil(sqrt(n)), but never more than n/2 (need at least 2 nodes per cluster)
    int max_k = (int)ceil(sqrt(n));
    max_k = min(max_k, n / 2);
    
    if (rank == 0) {
        cout << "Finding optimal cluster count (testing k=2 to " << max_k << ")..." << endl;
    }
    
    double best_score = -1e100;
    int best_k = 2;
    
    for (int k = 2; k <= max_k; ++k) {
        vector<vector<int>> clusters;
        bool ok = algo1_best_effort_weighted_mpi(G, k, clusters, rank, size);
        
        if (rank != 0) continue; // Only rank 0 evaluates
        
        if (!ok || clusters.empty()) continue;
        
        // Compute quality metrics
        double mean_conductance = 0.0;
        double mean_expansion = 0.0;
        int minsz = INT_MAX, maxsz = 0;
        
        for (const auto& C : clusters) {
            mean_conductance += external_conductance_weighted(G, C);
            mean_expansion += spectral_gap_proxy_weighted(G, C);
            minsz = min(minsz, (int)C.size());
            maxsz = max(maxsz, (int)C.size());
        }
        
        mean_conductance /= clusters.size();
        mean_expansion /= clusters.size();
        
        // Balance score: penalize imbalance
        double balance = (double)minsz / max(1.0, (double)maxsz);
        
        // Quality score, we prefer low conductance, high expansion, good balance
        // Weight expansion more heavily as it indicates good internal connectivity
        double score = (1.0 - mean_conductance) * 0.3 + mean_expansion * 0.5 + balance * 0.2;
        
        if (score > best_score) {
            best_score = score;
            best_k = k;
        }
    }
    
    // Broadcast best_k to all ranks
    MPI_Bcast(&best_k, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    if (rank == 0) {
        cout << "Selected optimal k = " << best_k << endl;
    }
    
    return best_k;
}

int main(int argc, char** argv){
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int n = 0, m = 0;
    int k = 0;
    vector<Edge> raw_edges;
    string input_file, output_prefix;
    bool auto_k = false;

    // Parse command line arguments
    if (rank == 0) {
        if (argc != 3) {
            cerr << "Usage: " << argv[0] << " <input.top> <output_prefix>" << endl;
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        input_file = argv[1];
        output_prefix = argv[2];
        
        // Read topology file
        ifstream file(input_file);
        if (!file.is_open()) {
            cerr << "ERROR: Cannot open input file: " << input_file << endl;
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        
        // Read first line: n m
        if (!(file >> n >> m)) {
            cerr << "ERROR: Cannot read n, m from file" << endl;
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        
        cout << "Reading topology: " << n << " vertices, " << m << " edges" << endl;
        
        // Read edges
        raw_edges.reserve(m);
        string line;
        getline(file, line); // consume rest of first line
        
        while (getline(file, line)) {
            if (line.empty() || line.find_first_not_of(" \t\r\n") == string::npos) continue;
            
            // Replace commas with spaces
            for (char& c : line) {
                if (c == ',') c = ' ';
            }
            
            stringstream ss(line);
            int u, v;
            double w = 1.0;
            
            if (!(ss >> u >> v)) continue;
            ss >> w; // optional weight
            
            Edge e; e.u = u; e.v = v; e.w = w;
            raw_edges.push_back(e);
        }
        file.close();
        
        m = raw_edges.size(); // Update m to actual edge count
        
        // We'll determine k automatically
        auto_k = true;
    }

    // broadcast n, m
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&m, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank != 0){
        raw_edges.resize(m);
    }

    // broadcast edges as bytes
    if (m > 0){
        MPI_Bcast(reinterpret_cast<char*>(raw_edges.data()), m * (int)sizeof(Edge), MPI_BYTE, 0, MPI_COMM_WORLD);
    }

    // all ranks build same Graph
    bool one_based = false;
    if (rank == 0){
        one_based = is_one_based_with_edges(raw_edges, n);
    }
    MPI_Bcast(&one_based, 1, MPI_C_BOOL, 0, MPI_COMM_WORLD);

    Graph G(n);
    for (auto &e: raw_edges){
        int u = e.u, v = e.v;
        double w = e.w;
        if (one_based){ u--; v--; }
        if (u < 0 || u >= n || v < 0 || v >= n) continue;
        if (w <= 0) continue;
        G.add_edge(u, v, w);
    }

    // Determine optimal k
    k = find_optimal_k(G, rank, size);
    
    if (rank == 0){
        cout << "Running clustering..." << endl;
    }

    MPI_Barrier(MPI_COMM_WORLD);
    Timer t1;
    if (rank == 0) t1.start();

    vector<vector<int>> clusters1;
    bool ok1 = algo1_best_effort_weighted_mpi(G, k, clusters1, rank, size);

    MPI_Barrier(MPI_COMM_WORLD);
    double t1ms = 0.0;
    if (rank == 0) t1ms = t1.stop_ms();

    if (rank == 0){
        if (!ok1) {
            cout << "ERROR: Clustering failed" << endl;
        } else {
            // Don't print detailed cluster assignments - they're in the .output file
            auto M1 = evaluate_partition_weighted(G, clusters1, t1ms);
            
            // Write output files first
            string output_file = output_prefix + ".output";
            string stats_file = output_prefix + ".stats";
            
            ofstream out(output_file);
            if (out.is_open()) {
                out << clusters1.size() << "\n"; // actual k
                for (size_t i = 0; i < clusters1.size(); ++i) {
                    for (size_t j = 0; j < clusters1[i].size(); ++j) {
                        if (j > 0) out << ",";
                        out << (clusters1[i][j] + 1); // 1-based output
                    }
                    out << "\n";
                }
                out.close();
            } else {
                cerr << "ERROR: Cannot write to output file: " << output_file << endl;
            }
            
            write_metrics_to_file(M1, stats_file);
            
            // Print compact summary
            cout << "\nClustering Results (k=" << clusters1.size() << "):" << endl;
            cout << "  Runtime: " << fixed << setprecision(2) << M1.runtime_ms << " ms" << endl;
            cout << "  Cut edges: " << fixed << setprecision(1) << M1.cut_edges << " (" << fixed << setprecision(2) << (100.0 * M1.cut_edges / max(1e-12, M1.total_edges)) << "% of total)" << endl;
            cout << "  External conductance: " << fixed << setprecision(4) << (M1.ext_conductance.empty() ? 0.0 : accumulate(M1.ext_conductance.begin(), M1.ext_conductance.end(), 0.0) / M1.ext_conductance.size()) << endl;
            cout << "  Internal expansion: " << fixed << setprecision(4) << (M1.gap1_minus_mu2.empty() ? 0.0 : accumulate(M1.gap1_minus_mu2.begin(), M1.gap1_minus_mu2.end(), 0.0) / M1.gap1_minus_mu2.size()) << endl;
            
            cout << "\nOutput files generated:" << endl;
            cout << "  " << output_file << " (cluster assignments)" << endl;
            cout << "  " << stats_file << " (detailed metrics)" << endl;
        }
    }

    MPI_Finalize();
    return 0;
}
