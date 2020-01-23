/**
 * Copyright (C) 2015 by Liangliang Nan (liangliang.nan@gmail.com)
 * https://3d.bk.tudelft.nl/liangliang/
 *
 * This file is part of Easy3D. If it is useful in your research/work,
 * I would be grateful if you show your appreciation by citing it:
 * ------------------------------------------------------------------
 *      Liangliang Nan.
 *      Easy3D: a lightweight, easy-to-use, and efficient C++
 *      library for processing and rendering 3D data. 2018.
 * ------------------------------------------------------------------
 * Easy3D is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 3
 * as published by the Free Software Foundation.
 *
 * Easy3D is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <easy3d/core/manifold_guard.h>
#include <easy3d/util/logging.h>


namespace easy3d {

    ManifoldGuard::ManifoldGuard(SurfaceMesh *mesh)
    : mesh_(mesh)
    {
    }


    ManifoldGuard::~ManifoldGuard() {

    }


    void ManifoldGuard::begin() {
        num_faces_less_three_vertices_ = 0;
        num_faces_duplicated_vertices_= 0;
        num_non_manifold_edges_= 0;
        num_non_manifold_vertices_= 0;
        num_isolated_vertices_= 0;

        input_face_vertices_.clear();
        face_vertices_.clear();

        copies_.clear();
    }


    void ManifoldGuard::finish() {
        std::string msg = "mesh has topological issues:";
        bool report(false);

        for (auto v : mesh_->vertices()) {
            if (mesh_->is_isolated(v)) {
                mesh_->delete_vertex(v);
                ++num_isolated_vertices_;
            }
        }
        mesh_->garbage_collection();

        if (num_isolated_vertices_ > 0) {
            msg += "\n\t" + std::to_string(num_isolated_vertices_) + " isolated vertices (removed)";
            report = true;
        }

        if (num_faces_less_three_vertices_ > 0) {
            msg += "\n\t" + std::to_string(num_faces_less_three_vertices_) +
                      " faces with less than 3 vertices (ignored)";
            report = true;
        }
        if (num_faces_duplicated_vertices_ > 0) {
            msg += "\n\t" + std::to_string(num_faces_duplicated_vertices_) +
                      " faces with duplicated vertices (ignored)";
            report = true;
        }
        if (num_non_manifold_edges_ > 0) {
            msg += "\n\t" + std::to_string(num_non_manifold_edges_) + " non_manifold edges (fixed)";
            report = true;
        }

        for (auto v : mesh_->vertices()) {
            if (!mesh_->is_manifold(v))
                ++num_non_manifold_vertices_;
        }
        if (num_non_manifold_vertices_ > 0) {
            msg += "\n\t" + std::to_string(num_non_manifold_vertices_) + " non_manifold vertices (not fixed)";
            report = true;
        }

#if 0
        for (auto g : copies_) {
            msg += "\n\tvertex v" + std::to_string(g.first.idx()) + " copied to: ";
            for (auto v : g.second)
                msg += "v" + std::to_string(v.idx()) + " ";
        }
#endif

        LOG_IF(WARNING, report) << msg;
    }


    SurfaceMesh::Vertex ManifoldGuard::add_vertex(const vec3 &p) {
        return mesh_->add_vertex(p);
    }


    SurfaceMesh::Face ManifoldGuard::add_face(const std::vector<unsigned int> &vertices) {
        std::size_t nb_vertices = vertices.size();

        // check if a face has less than 3 vertices;
        if (nb_vertices < 3) {
            ++num_faces_less_three_vertices_;
            return SurfaceMesh::Face();
        }

        // check if a face has duplicated vertices
        for (std::size_t i = 0; i < nb_vertices; ++i) {
            for (std::size_t j = i+1; j < nb_vertices; ++j) {
                if (vertices[i] == vertices[j]) {
                    ++num_faces_duplicated_vertices_;
                    return SurfaceMesh::Face();
                }
            }
        }

        input_face_vertices_.resize(nb_vertices);
        face_vertices_.resize(nb_vertices);
        for (std::size_t i = 0; i < nb_vertices; ++i)
            input_face_vertices_[i] = face_vertices_[i] = SurfaceMesh::Vertex(vertices[i]);

//        std::cout << "----------------------------\n";
//        std::cout << "original vertices: " << face_vertices_ << std::endl;

        // Detect and remove non-manifold edges by duplicating the corresponding vertices.
        for (std::size_t s = 0; s < nb_vertices; s++) {
            std::size_t t = ((s + 1) % nb_vertices);
            find_or_duplicate_edge(s, t);
            //std::cout << "edge is ok: " << face_vertices_[s] << " -> " << face_vertices_[t] << std::endl;
        }

        auto face = mesh_->add_face(face_vertices_);
//        if (!face.is_valid())
//            std::cout << "   ----- failed add face: " << face_vertices_ << std::endl;
//        else
//            std::cout << "   ----- face added: " << face_vertices_ << std::endl;
        return face;
    }


    void ManifoldGuard::find_or_duplicate_edge(unsigned int s, unsigned int t) {
        // For the edge (face_vertices_[s] -> face_vertices_[t]) of the current face,
        // check if adding this edge can result in an complex edge.

        if (!halfedge_is_legel(face_vertices_[s], face_vertices_[t])) { // complex edge expected
            ++num_non_manifold_edges_;
//            std::cout << " -- bad edge (s <-> t): " << face_vertices_[s] << " <-> " << face_vertices_[t] << std::endl;

            // Check the copied vertices until we can find one.

            // for the copies of s
            if (copies_.find(face_vertices_[s]) != copies_.end()) { // s has copies
                for (auto v : copies_[face_vertices_[s]]) {
                    if (halfedge_is_legel(v, face_vertices_[t])) {
                        face_vertices_[s] = v;
                        //std::cout << "edge is ok: " << face_vertices_[s] << " -> " << face_vertices_[t] << std::endl;
                        return;
                    }
//                    else
//                        std::cout << " -- bad edge (s <-> t): " << face_vertices_[s] << " <-> " << face_vertices_[t] << std::endl;
                }
            }

            // for the copies of t
            if (copies_.find(face_vertices_[t]) != copies_.end()) { // s has copies
                for (auto v : copies_[face_vertices_[t]]) {
                    if (halfedge_is_legel(face_vertices_[s], v)) {
                        face_vertices_[t] = v;
                        //std::cout << "edge is ok: " << face_vertices_[s] << " -> " << face_vertices_[t] << std::endl;
                        return;
                    }
//                    else
//                        std::cout << " -- bad edge (s <-> t): " << face_vertices_[s] << " <-> " << face_vertices_[t] << std::endl;
                }
            }

            // for the copies of both s and t
            if (copies_.find(face_vertices_[s]) != copies_.end() && copies_.find(face_vertices_[t]) != copies_.end()) { // both s and t have copies
                for (auto vs : copies_[face_vertices_[s]]) {
                    for (auto vt : copies_[face_vertices_[t]]) {
                        if (halfedge_is_legel(vs, vt)) {
                            face_vertices_[s] = vs;
                            face_vertices_[t] = vt;
                            //std::cout << "edge is ok: " << face_vertices_[s] << " -> " << face_vertices_[t] << std::endl;
                            return;
                        }
//                        else
//                            std::cout << " -- bad edge (s <-> t): " << face_vertices_[s] << " <-> " << face_vertices_[t] << std::endl;
                    }
                }
            }

            //std::cout << " -- bad edge (s <-> t): " << face_vertices_[s] << " <-> " << face_vertices_[t] << std::endl;
            // If we reach here, we must copy at least one of s and t. We try to copy the closed disk one first.
            if (!mesh_->is_boundary(face_vertices_[s])) {
                face_vertices_[s] = copy_vertex(input_face_vertices_[s]);
                if (halfedge_is_legel(face_vertices_[s], face_vertices_[t]))
                    return;
            }
            if (!mesh_->is_boundary(face_vertices_[t])) {
                face_vertices_[t] = copy_vertex(input_face_vertices_[t]);
                if (halfedge_is_legel(face_vertices_[s], face_vertices_[t]))
                    return;
            }

            // It must be a very complex situation if we reach here: we simple copy both.
            if (face_vertices_[s] == input_face_vertices_[s])
                face_vertices_[s] = copy_vertex(input_face_vertices_[s]);
            if (face_vertices_[t] == input_face_vertices_[t])
                face_vertices_[t] = copy_vertex(input_face_vertices_[t]);
        }
    }


    bool ManifoldGuard::halfedge_is_legel(SurfaceMesh::Vertex s, SurfaceMesh::Vertex t) const {
        auto h = mesh_->find_halfedge(s, t);

        // the edge does not exist or it is a boundary (i.e., the face is NULL)
        if (h.is_valid() && !mesh_->is_boundary(h))
            return false;

        // one of the vertices is a closed disk
        if (!mesh_->is_boundary(s) || !mesh_->is_boundary(t))
            return false;

        return true;
    }


    SurfaceMesh::Vertex ManifoldGuard::copy_vertex(SurfaceMesh::Vertex v) {
        auto points = mesh_->vertex_property<vec3>("v:point");
        auto new_v = mesh_->add_vertex(points[v]);
        copies_[v].push_back(new_v);
        //std::cout << "vertex " << v << " copied to " << new_v << std::endl;
        return new_v;
    }

}
