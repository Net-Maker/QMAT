// @author: CookMaker and Claude 4 Sonnet 
// @date: 2025-07-07
// @description: This is a simple converter from mesh to medial axis.

#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <vector>
#include <set>
#include <map>

// Include necessary headers
#include "Mesh.h"
#include "ThreeDimensionalShape.h"

using namespace std;

class MeshToMAConverter {
private:
    unique_ptr<ThreeDimensionalShape> shape;
    string input_filename;
    string output_filename;
    Polyhedron polyhedron; // This is *so* important to be defined here, cause the class should stay in memory, or would cause segfault bt illigal memory access
    
    // 简单的标签存储 - 每个mesh顶点一个label
    std::vector<int> mesh_vertex_labels;              // mesh顶点的标签，索引对应pVertexList
    
    // MA顶点的多标签存储
    std::vector<std::set<int>> ma_vertex_labels;      // 每个MA顶点的标签集合
    
    // 可选的标签元信息
    std::map<int, std::string> label_names;           // 标签ID到名称映射（可选）

    // 边界点到原始顶点的简单映射
    std::vector<int> boundary_point_to_mesh_vertex;   // 每个边界点对应的mesh顶点索引
    
    // 可选：反向映射用于验证
    std::map<int, std::vector<int>> mesh_vertex_to_boundary_points; // mesh顶点到边界点的映射

public:
    MeshToMAConverter(const string& input_file, const string& output_file = "") 
        : input_filename(input_file) {
        if (output_file.empty()) {
            // Generate output filename by replacing .off with .ma
            size_t dot_pos = input_file.find_last_of('.');
            if (dot_pos != string::npos) {
                output_filename = input_file.substr(0, dot_pos) + ".ma";
            } else {
                output_filename = input_file + ".ma";
            }
        } else {
            output_filename = output_file;
        }
        shape = make_unique<ThreeDimensionalShape>();
    }

    bool loadMesh() {
        cout << "Loading mesh file: " << input_filename << endl;
        
        // Load the mesh file
        ifstream stream(input_filename);
        if (!stream) {
            cerr << "Error: Cannot open input file " << input_filename << endl;
            return false;
        }

        try {
            // Read mesh data
            stream >> shape->input;
            
            // Compute basic mesh properties
            shape->input.computebb();           // Compute bounding box
            shape->input.GenerateList();        // Generate vertex and triangle lists
            shape->input.GenerateRandomColor(); // Generate colors for visualization
            
            cout << "Mesh loaded successfully." << endl;
            return true;
        } catch (const exception& e) {
            cerr << "Error loading mesh: " << e.what() << endl;
            return false;
        }
    }

    bool setupDomain() {
        cout << "Setting up mesh domain..." << endl;
        
        try {
            ifstream streampol(input_filename);
            if (!streampol) {
                cerr << "Error: Cannot reopen input file for polyhedron" << endl;
                return false;
            }
            
            streampol >> polyhedron;
            
            Mesh_domain* pdom = new Mesh_domain(polyhedron);
            shape->input.domain = pdom;
            
            // Setup non-manifold mesh properties
            shape->input_nmm.domain = shape->input.domain;
            shape->input_nmm.pmesh = &(shape->input);
            shape->input_nmm.meshname = output_filename;
            
            // Setup slab mesh properties
            shape->slab_mesh.pmesh = &(shape->input);
            shape->slab_mesh.type = 1;
            
            cout << "Domain setup completed." << endl;
            return true;
        } catch (const exception& e) {
            cerr << "Error setting up domain: " << e.what() << endl;
            return false;
        }
    }

    bool computeMedialAxis() {
        cout << "Computing Delaunay triangulation..." << endl;
        
        try {
            // Compute Delaunay triangulation
            shape->input.computedt();
            cout << "Delaunay triangulation completed." << endl;
            
            cout << "Marking poles..." << endl;
            // Mark poles in the surface
            shape->input.markpoles();
            cout << "Pole marking completed." << endl;
            
            cout << "Computing medial axis..." << endl;
            // Compute the input non-manifold mesh (medial axis)
            shape->ComputeInputNMM();
            cout << "Medial axis computation completed." << endl;
            
            // 添加标签传递
            if (mesh_vertex_labels.empty() == false) {
                cout << "Building boundary point mapping..." << endl;
                buildBoundaryPointMapping();
                
                cout << "Propagating labels to medial axis..." << endl;
                propagateLabelsToMA();
                cout << "Label propagation completed." << endl;
            }
            
            cout << "Pruning slab mesh..." << endl;
            // Prune the slab mesh
            shape->PruningSlabMesh();
            cout << "Slab mesh pruning completed." << endl;
            
            return true;
        } catch (const exception& e) {
            cerr << "Error computing medial axis: " << e.what() << endl;
            return false;
        }
    }

    bool exportMA() {
        cout << "Exporting medial axis to: " << output_filename << endl;
        
        try {
            // Export the computed medial axis
            shape->input_nmm.Export(shape->input_nmm.meshname);
            cout << "Medial axis exported successfully." << endl;
            return true;
        } catch (const exception& e) {
            cerr << "Error exporting medial axis: " << e.what() << endl;
            return false;
        }
    }

    bool exportMAWithLabels() {
        if (ma_vertex_labels.empty()) {
            return exportMA(); // 回退到原始导出
        }
        
        // 导出原始MA
        if (!exportMA()) return false;
        
        // 导出标签信息
        string label_output = output_filename + ".labels";
        ofstream label_file(label_output);
        if (!label_file) return false;
        
        label_file << "# MA Vertex Labels" << endl;
        label_file << "# Format: vertex_index label1 label2 ..." << endl;
        
        for (size_t i = 0; i < ma_vertex_labels.size(); i++) {
            if (!ma_vertex_labels[i].empty()) {
                label_file << i;
                for (int label : ma_vertex_labels[i]) {
                    label_file << " " << label;
                }
                label_file << endl;
            }
        }
        
        label_file.close();
        cout << "Labels exported to: " << label_output << endl;
        return true;
    }

    void printStatistics() {
        cout << "\n=== Processing Statistics ===" << endl;
        cout << "Input mesh vertices: " << shape->input.pVertexList.size() << endl;
        cout << "Input mesh faces: " << shape->input.pFaceList.size() << endl;
        cout << "Medial axis vertices: " << shape->input_nmm.numVertices << endl;
        cout << "Medial axis edges: " << shape->input_nmm.numEdges << endl;
        cout << "Medial axis faces: " << shape->input_nmm.numFaces << endl;
        cout << "Slab mesh vertices: " << shape->slab_mesh.numVertices << endl;
        cout << "Slab mesh edges: " << shape->slab_mesh.numEdges << endl;
        cout << "Slab mesh faces: " << shape->slab_mesh.numFaces << endl;
        cout << "============================" << endl;
    }

    bool process() {
        cout << "Starting mesh to medial axis conversion..." << endl;
        
        if (!loadMesh()) return false;
        if (!setupDomain()) return false;
        if (!computeMedialAxis()) return false;
        if (!exportMA()) return false;
        
        printStatistics();
        cout << "Conversion completed successfully!" << endl;
        return true;
    }

    // 加载mesh顶点标签
    bool loadMeshVertexLabels(const std::string& label_file) {
        std::ifstream file(label_file);
        if (!file) return false;
        
        mesh_vertex_labels.resize(shape->input.pVertexList.size());
        
        // 简单格式：每行一个标签值，按顶点索引顺序
        for (size_t i = 0; i < mesh_vertex_labels.size(); i++) {
            file >> mesh_vertex_labels[i];
        }
        return true;
    }
    
    // 建立边界点映射关系
    void buildBoundaryPointMapping() {
        boundary_point_to_mesh_vertex.clear();
        boundary_point_to_mesh_vertex.resize(shape->input_nmm.BoundaryPoints.size());
        
        // 遍历边界点，找到最近的mesh顶点
        for (size_t bp_idx = 0; bp_idx < shape->input_nmm.BoundaryPoints.size(); bp_idx++) {
            const auto& bp = shape->input_nmm.BoundaryPoints[bp_idx];
            
            double min_dist = std::numeric_limits<double>::max();
            int closest_vertex = -1;
            
            // 找最近的mesh顶点
            for (size_t v_idx = 0; v_idx < shape->input.pVertexList.size(); v_idx++) {
                auto vertex = shape->input.pVertexList[v_idx];
                double dist = sqrt(
                    pow(vertex->point().x() - bp.X(), 2) +
                    pow(vertex->point().y() - bp.Y(), 2) +
                    pow(vertex->point().z() - bp.Z(), 2)
                );
                
                if (dist < min_dist) {
                    min_dist = dist;
                    closest_vertex = v_idx;
                }
            }
            
            boundary_point_to_mesh_vertex[bp_idx] = closest_vertex;
        }
    }
    
    // 传递标签到MA顶点
    void propagateLabelsToMA() {
        ma_vertex_labels.clear();
        ma_vertex_labels.resize(shape->input_nmm.numVertices);
        
        // 遍历每个MA顶点
        for (size_t ma_v_idx = 0; ma_v_idx < shape->input_nmm.vertices.size(); ma_v_idx++) {
            if (!shape->input_nmm.vertices[ma_v_idx].first) continue;
            
            auto ma_vertex = shape->input_nmm.vertices[ma_v_idx].second;
            
            // 获取这个MA顶点关联的边界点
            for (int bp_idx : ma_vertex->bplist) {
                if (bp_idx < boundary_point_to_mesh_vertex.size()) {
                    int mesh_v_idx = boundary_point_to_mesh_vertex[bp_idx];
                    if (mesh_v_idx >= 0 && mesh_v_idx < mesh_vertex_labels.size()) {
                        // 添加对应mesh顶点的标签
                        ma_vertex_labels[ma_v_idx].insert(mesh_vertex_labels[mesh_v_idx]);
                    }
                }
            }
        }
    }

    // 处理已加载mesh的剩余步骤
    bool processWithLoadedMesh() {
        cout << "Continuing mesh to medial axis conversion..." << endl;
        
        if (!setupDomain()) return false;
        if (!computeMedialAxis()) return false;
        if (!exportMAWithLabels()) return false;  // 使用带标签的导出
        
        printStatistics();
        cout << "Conversion completed successfully!" << endl;
        return true;
    }
};

void printUsage(const string& program_name) {
    cout << "Usage: " << program_name << " <input_mesh.off> [output_file.ma]" << endl;
    cout << "  input_mesh.off  - Input mesh file in OFF format" << endl;
    cout << "  output_file.ma  - Optional output MA file (default: input_name.ma)" << endl;
    cout << endl;
    cout << "Example: " << program_name << " bunny.off bunny_ma.ma" << endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2 || argc > 4) {
        printUsage(argv[0]);
        return 1;
    }

    string input_file = argv[1];
    string output_file = (argc == 3) ? argv[2] : "";

    // Check if input file exists
    ifstream test_file(input_file);
    if (!test_file) {
        cerr << "Error: Input file '" << input_file << "' does not exist or cannot be opened." << endl;
        return 1;
    }
    test_file.close();

    try {
        MeshToMAConverter converter(input_file, output_file);
        
        // 先加载mesh
        if (!converter.loadMesh()) {
            cerr << "Error: Failed to load mesh." << endl;
            return 1;
        }
        
        // 可选：加载顶点标签
        if (argc > 3) {
            string label_file = argv[3];
            if (!converter.loadMeshVertexLabels(label_file)) {
                cerr << "Warning: Could not load vertex labels from " << label_file << endl;
            } else {
                cout << "Labels loaded successfully." << endl;
            }
        }
        
        // 继续处理其余步骤
        if (converter.processWithLoadedMesh()) {
            cout << "Success: Medial axis computation completed." << endl;
            return 0;
        } else {
            cerr << "Error: Medial axis computation failed." << endl;
            return 1;
        }
    } catch (const exception& e) {
        cerr << "Fatal error: " << e.what() << endl;
        return 1;
    }
} 