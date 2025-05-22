#include <iostream>

#include "src/NonManifoldMesh.h"
#include "src/SlabMesh.h"

void LoadInputNMM(Mesh* input, SlabMesh* slabMesh, std::string maname) {
  std::ifstream mastream(maname.c_str());
  NonManifoldMesh newinputnmm;
  newinputnmm.numVertices = 0;
  newinputnmm.numEdges = 0;
  newinputnmm.numFaces = 0;
  int nv, ne, nf;
  mastream >> nv >> ne >> nf;

  // slab mesh
  slabMesh->numVertices = 0;
  slabMesh->numEdges = 0;
  slabMesh->numFaces = 0;
  slabMesh->bound_weight = 0.1;

  double len[4];
  len[0] = input->m_max[0] - input->m_min[0];
  len[1] = input->m_max[1] - input->m_min[1];
  len[2] = input->m_max[2] - input->m_min[2];
  len[3] = sqrt(len[0] * len[0] + len[1] * len[1] + len[2] * len[2]);
  newinputnmm.diameter = len[3];

  for (unsigned i = 0; i < input->pVertexList.size(); i++)
    newinputnmm.BoundaryPoints.push_back(SamplePoint(
        input->pVertexList[i]->point()[0], input->pVertexList[i]->point()[1],
        input->pVertexList[i]->point()[2]));

  for (unsigned i = 0; i < nv; i++) {
    char ch;
    double x, y, z, r;
    mastream >> ch >> x >> y >> z >> r;

    // handle the slab mesh
    Bool_SlabVertexPointer bsvp2;
    bsvp2.first = true;
    bsvp2.second = new SlabVertex;
    (*bsvp2.second).sphere.center[0] = x / input->bb_diagonal_length;
    (*bsvp2.second).sphere.center[1] = y / input->bb_diagonal_length;
    (*bsvp2.second).sphere.center[2] = z / input->bb_diagonal_length;
    (*bsvp2.second).sphere.radius = r / input->bb_diagonal_length;
    (*bsvp2.second).index = slabMesh->vertices.size();
    slabMesh->vertices.push_back(bsvp2);
    slabMesh->numVertices++;
  }

  for (unsigned i = 0; i < ne; i++) {
    char ch;
    unsigned ver[2];
    mastream >> ch;
    mastream >> ver[0];
    mastream >> ver[1];

    // handle the slab mesh
    Bool_SlabEdgePointer bsep2;
    bsep2.first = true;
    bsep2.second = new SlabEdge;
    (*bsep2.second).vertices_.first = ver[0];
    (*bsep2.second).vertices_.second = ver[1];
    (*slabMesh->vertices[(*bsep2.second).vertices_.first].second)
        .edges_.insert(slabMesh->edges.size());
    (*slabMesh->vertices[(*bsep2.second).vertices_.second].second)
        .edges_.insert(slabMesh->edges.size());
    (*bsep2.second).index = slabMesh->edges.size();
    slabMesh->edges.push_back(bsep2);
    slabMesh->numEdges++;
  }

  for (unsigned i = 0; i < nf; i++) {
    char ch;
    unsigned vid[3];
    unsigned eid[3];
    mastream >> ch >> vid[0] >> vid[1] >> vid[2];

    // handle the slab mesh
    Bool_SlabFacePointer bsfp2;
    bsfp2.first = true;
    bsfp2.second = new SlabFace;
    (*bsfp2.second).vertices_.insert(vid[0]);
    (*bsfp2.second).vertices_.insert(vid[1]);
    (*bsfp2.second).vertices_.insert(vid[2]);
    if (slabMesh->Edge(vid[0], vid[1], eid[0]))
      (*bsfp2.second).edges_.insert(eid[0]);
    if (slabMesh->Edge(vid[0], vid[2], eid[1]))
      (*bsfp2.second).edges_.insert(eid[1]);
    if (slabMesh->Edge(vid[1], vid[2], eid[2]))
      (*bsfp2.second).edges_.insert(eid[2]);
    (*bsfp2.second).index = slabMesh->faces.size();
    slabMesh->vertices[vid[0]].second->faces_.insert(slabMesh->faces.size());
    // slab_mesh.vertices[vid[0]].second->related_face += 2;
    slabMesh->vertices[vid[1]].second->faces_.insert(slabMesh->faces.size());
    // slab_mesh.vertices[vid[1]].second->related_face += 2;
    slabMesh->vertices[vid[2]].second->faces_.insert(slabMesh->faces.size());
    // slab_mesh.vertices[vid[2]].second->related_face += 2;
    slabMesh->edges[eid[0]].second->faces_.insert(slabMesh->faces.size());
    slabMesh->edges[eid[1]].second->faces_.insert(slabMesh->faces.size());
    slabMesh->edges[eid[2]].second->faces_.insert(slabMesh->faces.size());
    slabMesh->faces.push_back(bsfp2);
    slabMesh->numFaces++;
  }

  // newinputnmm.ComputeFacesNormal();
  // newinputnmm.ComputeFacesCentroid();
  // newinputnmm.ComputeFacesSimpleTriangles();
  // newinputnmm.ComputeEdgesCone();
  // input_nmm = newinputnmm;

  slabMesh->iniNumVertices = slabMesh->numVertices;
  slabMesh->iniNumEdges = slabMesh->numEdges;
  slabMesh->iniNumFaces = slabMesh->numFaces;

  slabMesh->CleanIsolatedVertices();
  slabMesh->computebb();
  slabMesh->ComputeFacesCentroid();
  slabMesh->ComputeFacesNormal();
  slabMesh->ComputeVerticesNormal();
  slabMesh->ComputeEdgesCone();
  slabMesh->ComputeFacesSimpleTriangles();
  slabMesh->DistinguishVertexType();
}

bool importMA(Mesh* input, SlabMesh* slabMesh, std::string maname) {
  // std::string filename = filename + ".ma";
  // if (!std::filesystem::exists(filename)) {
  //     std::cerr << "Related .ma file is missing." << std::endl;
  //     return false;
  // }

  std::cout << "Loading ma file " << maname << std::endl;
  // bool success = false;

  // m_pThreeDimensionalShape->input_nmm.meshname = filename;
  // m_pThreeDimensionalShape->input_nmm.domain =
  // m_pThreeDimensionalShape->input.domain;
  // m_pThreeDimensionalShape->input_nmm.pmesh =
  // &(m_pThreeDimensionalShape->input);
  // m_pThreeDimensionalShape->slab_mesh.pmesh =
  // &(m_pThreeDimensionalShape->input);
  slabMesh->type = 1;
  slabMesh->bound_weight = 1.0;
  // m_pThreeDimensionalShape->slab_mesh.type = 1;
  // m_pThreeDimensionalShape->slab_mesh.bound_weight = 1.0;
  // m_pThreeDimensionalShape->LoadInputNMM(filename);
  LoadInputNMM(input, slabMesh, maname);
  std::cout << "import MA done." << std::endl;
  // success = true;

  // if (success) {
  //     m_pGLWidget->set3DShape(m_pThreeDimensionalShape);
  // }

  return true;
}

void LoadSlabMesh(SlabMesh* slabMesh) {
  slabMesh->clear();
  // long startt = clock();
  // handle each face
  for (unsigned i = 0; i < slabMesh->vertices.size(); i++) {
    if (!slabMesh->vertices[i].first) continue;

    SlabVertex sv = *slabMesh->vertices[i].second;
    std::set<unsigned> fset = sv.faces_;
    Vector4d C1(sv.sphere.center.X(), sv.sphere.center.Y(),
                sv.sphere.center.Z(), sv.sphere.radius);

    for (set<unsigned>::iterator si = fset.begin(); si != fset.end(); si++) {
      SlabFace sf = *slabMesh->faces[*si].second;

      if (sf.valid_st == false || sf.st[0].normal == Vector3d(0., 0., 0.) ||
          sf.st[1].normal == Vector3d(0., 0., 0.))
        continue;

      Vector4d normal1(sf.st[0].normal.X(), sf.st[0].normal.Y(),
                       sf.st[0].normal.Z(), 1.0);
      Vector4d normal2(sf.st[1].normal.X(), sf.st[1].normal.Y(),
                       sf.st[1].normal.Z(), 1.0);

      // compute the matrix of A
      Matrix4d temp_A1, temp_A2;
      temp_A1.MakeTensorProduct(normal1, normal1);
      temp_A2.MakeTensorProduct(normal2, normal2);
      temp_A1 *= 2.0;
      temp_A2 *= 2.0;

      // compute the matrix of b
      double normal_mul_point1 = normal1.Dot(C1);
      double normal_mul_point2 = normal2.Dot(C1);
      Wm4::Vector4d temp_b1 = normal1 * 2 * normal_mul_point1;
      Wm4::Vector4d temp_b2 = normal2 * 2 * normal_mul_point2;

      // compute c
      double temp_c1 = normal_mul_point1 * normal_mul_point1;
      double temp_c2 = normal_mul_point2 * normal_mul_point2;

      slabMesh->vertices[i].second->slab_A += temp_A1;
      slabMesh->vertices[i].second->slab_A += temp_A2;
      slabMesh->vertices[i].second->slab_b += temp_b1;
      slabMesh->vertices[i].second->slab_b += temp_b2;
      slabMesh->vertices[i].second->slab_c += temp_c1;
      slabMesh->vertices[i].second->slab_c += temp_c2;

      slabMesh->vertices[i].second->related_face += 2;
    }
  }

  switch (slabMesh->preserve_boundary_method) {
    case 1:
      slabMesh->PreservBoundaryMethodOne();
      break;
    case 2:
      // slab_mesh.PreservBoundaryMethodTwo();
      break;
    case 3:
      slabMesh->PreservBoundaryMethodThree();
      break;
    default:
      slabMesh->PreservBoundaryMethodFour();
      break;
  }

  slabMesh->initCollapseQueue();
  // long endt = clock();
}

void openmeshfile(Mesh* input, SlabMesh* slabMesh, std::string filename,
                  std::string maname) {
  // QString filename = QFileDialog::getOpenFileName(this, tr("Select a 3D model
  // to open"), NULL, tr("3D model(*.off)"));
  if (!filename.empty()) {
    // QDir qd(filename);
    std::string prefix = filename.substr(0, filename.size() - 4);
    // ThreeDimensionalShape * pThreeDimensionalShape = new
    // ThreeDimensionalShape;
    bool suc = false;
    std::ifstream stream(filename);

    if (stream) {
      stream >> *input;
      // compute the properties of the input mesh
      input->computebb();            // bounding box
      input->GenerateList();         // generate vertex and triangle list
      input->GenerateRandomColor();  // color of vertex and triangle
      input->compute_normals();      // normal of vertex and triangle
      // pThreeDimensionalShape->input_nmm.meshname = filename.;

      std::ifstream streampol(filename);
      Polyhedron pol;
      streampol >> pol;

      // set the non manifold mesh
      // Mesh_domain * pdom;
      // pdom = new Mesh_domain(pol);
      suc = true;
    }
    if (suc) {
      // m_pThreeDimensionalShape = pThreeDimensionalShape;
      // //ui.actionShow_Edge->setChecked(true);
      //
      // ui.actionShow_Face->setChecked(true);
      // ui.actionReverse_Orientation->setChecked(true);

      // importVP(prefix);
      bool re = importMA(input, slabMesh, maname);
      if (re == false) return;

      float k = 0.00001;
      slabMesh->k = k;
      // initialize();
      slabMesh->preserve_boundary_method = 0;
      slabMesh->hyperbolic_weight_type = 3;
      slabMesh->compute_hausdorff = false;
      slabMesh->boundary_compute_scale = 0;
      slabMesh->prevent_inversion = false;

      LoadSlabMesh(slabMesh);
      // long ti = m_pThreeDimensionalShape->LoadSlabMesh();
      // slab_initial = true;

      // // set back
      // ui.actionSet_k_value->setChecked(false);
      //
      // // show the input mesh in the dialog.
      // m_pGLWidget->set3DShape(m_pThreeDimensionalShape);
      // statusBar()->showMessage(filename + tr(" is loaded successfully.") );
      // setWindowTitle( tr("Medial Axis Simplification 3D - ") + filename );

      // m_isSimplified = false;
      std::cout << "openmeshfile done." << std::endl;
    } else {
    }
  } else {
    std::cout << "Filename is empty !" << std::endl;
  }
}

void simplifySlab(SlabMesh* slabMesh, Mesh* mesh, unsigned num_spheres) {
  slabMesh->CleanIsolatedVertices();
  // int threhold = min(10000, (int)(slabMesh->numVertices / 2));
  int threhold = num_spheres;

  // bool ok = true;
  // int simplifyNum = min(10000, (int)(slabMesh->numVertices / 2));
  // if (ok)
  //     threhold = simplifyNum;
  // else
  //     return;

  // if(slabMesh == NULL)
  //     return;

  // long start_time = clock();
  // slabMesh->initCollapseQueue();
  // slabMesh->initBoundaryCollapseQueue();
  slabMesh->Simplify(slabMesh->numVertices - threhold);
  // long end_time = clock();
  //
  // std::string res;
  // std::stringstream ss;
  // ss << end_time - start_time;
  // ss >> res;

  slabMesh->ComputeFacesNormal();
  slabMesh->ComputeVerticesNormal();
  slabMesh->ComputeEdgesCone();
  slabMesh->ComputeFacesSimpleTriangles();

  std::cout << "Simplify done." << std::endl;
}


void test_simplify_with_selected_pole(SlabMesh* slabMesh, Mesh* mesh, unsigned num_spheres, std::string selected_file_path, std::string output_file_path) {
  slabMesh->CleanIsolatedVertices();
  // int threhold = min(10000, (int)(slabMesh->numVertices / 2));
  int threhold = num_spheres;
  vector<vector<double> > selected_pole;

  // bool ok = true;
  // int simplifyNum = min(10000, (int)(slabMesh->numVertices / 2));
  // if (ok)
  //     threhold = simplifyNum;
  // else
  //     return;

  // if(slabMesh == NULL)
  //     return;

  // long start_time = clock();
  // slabMesh->initCollapseQueue();
  // slabMesh->initBoundaryCollapseQueue();
  slabMesh->Simplify_with_Selected_Pole(slabMesh->numVertices - threhold, selected_pole, selected_file_path, output_file_path);
  // long end_time = clock();
  //
  // std::string res;
  // std::stringstream ss;
  // ss << end_time - start_time;
  // ss >> res;

  slabMesh->ComputeFacesNormal();
  slabMesh->ComputeVerticesNormal();
  slabMesh->ComputeEdgesCone();
  slabMesh->ComputeFacesSimpleTriangles();

  std::cout << "Simplify with selected pole done." << std::endl;
}

int main(int argc, char** argv) {
  if (argc < 5) {
    std::cerr << "Usage: " << argv[0]
              << " <mode> <surface_mesh.off> <medial_mesh.ma> <num_target_spheres> [selected_points.txt]"
              << std::endl
              << "  mode: 1 - Regular simplification, 2 - Simplification with selected poles" 
              << std::endl;
    return 1;
  }
  
  int mode = atoi(argv[1]);
  std::string filename = argv[2];
  std::string maname = argv[3];
  unsigned num_spheres = atoi(argv[4]);
  
  printf("Mode: %d\n", mode);
  printf("Reading off file: %s\n", filename.c_str());
  printf("Reading ma file: %s\n", maname.c_str());
  printf("Target number of spheres: %u\n", num_spheres);

  Mesh input;
  Mesh* pinput = &input;
  SlabMesh slabMesh;
  SlabMesh* pslabMesh = &slabMesh;
  
  // 打开和读取网格文件
  openmeshfile(pinput, pslabMesh, filename, maname);
  printf("Done openmeshfile\n");
  
  // 根据模式选择简化方法
  if (mode == 1) {
    // 模式1：常规简化
    printf("Running regular simplification...\n");
    simplifySlab(pslabMesh, pinput, num_spheres);
    printf("Done regular simplification\n");
  } 
  else if (mode == 2) {
    // 模式2：使用选定极点的简化
    if (argc < 6) {
      std::cerr << "Error: Mode 2 requires selected_points.txt file path" << std::endl;
      return 1;
    }
    
    std::string selected_points_file = argv[5];
    std::string output_obj_file = "./data/test_all_poles.obj";  // 默认输出文件
    
    // 如果提供了输出文件路径，则使用它
    if (argc >= 7) {
      output_obj_file = argv[6];
    }
    
    printf("Selected points file: %s\n", selected_points_file.c_str());
    printf("Output OBJ file: %s\n", output_obj_file.c_str());
    printf("Running simplification with selected poles...\n");
    
    test_simplify_with_selected_pole(pslabMesh, pinput, num_spheres, 
                                   selected_points_file, output_obj_file);
    
    printf("Done simplification with selected poles\n");
  }
  else {
    std::cerr << "Error: Invalid mode. Use 1 for regular simplification or 2 for simplification with selected poles." << std::endl;
    return 1;
  }
  
  // 导出结果
  printf("Exporting results...\n");
  pslabMesh->Export_OBJ("./data/sim_MA", pinput);
  pslabMesh->Export("export_half", pinput);
  printf("Done export\n");

  return 0;
}
