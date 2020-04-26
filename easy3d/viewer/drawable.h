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

#ifndef EASY3D_DRAWABLE_H
#define EASY3D_DRAWABLE_H

#include <string>
#include <vector>

#include <easy3d/core/types.h>
#include <easy3d/viewer/state.h>


namespace easy3d {

    class Model;
    class Camera;
    class Texture;
    class VertexArrayObject;


    /**
     * @brief The base class for drawable objects. A drawable represent a set of points, line segments, or triangles.
     * @details A Drawable is an abstraction for "something that can be drawn" (i.e., drawable objects), e.g., a point
     *          point cloud, the surface of a mesh, the wireframe of a surface mesh.
     *          A drawable manages its rendering status and controls the upload of the data to the GPU.
     *          A drawable can live independently or be associated with a Model.
     * @related @class Model
     */

    class Drawable : public State {
    public:
        // three types of drawables (the values are the same as GL_POINTS, GL_LINES, and GL_TRIANGLES).
        enum Type {
            DT_POINTS = 0x0000,
            DT_LINES = 0x0001,
            DT_TRIANGLES = 0x0004
        };

    public:
        // a drawable can be stand-alone or attached to a model
        Drawable(const std::string &name = "unknown", Model *model = nullptr);
        virtual ~Drawable();

        /// Returns the type of the drawable.
        virtual Type type() const = 0;

        const std::string &name() const { return name_; }
        void set_name(const std::string n) { name_ = n; }

        /// the model to which the drawable is attached to (can be NULL).
        Model *model() { return model_; }
        const Model *model() const { return model_; }
        void set_model(Model *m) { model_ = m; }

        const Box3 &bounding_box() const { return bbox_; }

        /// print statistics (e.g., num vertices, memory usage) of the buffers to an output stream (e.g., std::cout).
        void buffer_stats(std::ostream &output) const;

        // ---------------------- buffer access ------------------------------

        unsigned int vertex_buffer() const { return vertex_buffer_; }
        unsigned int color_buffer() const { return color_buffer_; }
        unsigned int normal_buffer() const { return normal_buffer_; }
        unsigned int texcoord_buffer() const { return texcoord_buffer_; }
        unsigned int index_buffer() const { return index_buffer_; }
        unsigned int storage_buffer() const { return storage_buffer_; }
        unsigned int selection_buffer() const { return selection_buffer_; }

        // ------------------- buffer management ------------------------

        /// Creates/Updates a single buffer.
        void update_vertex_buffer(const std::vector<vec3> &vertices);
        void update_color_buffer(const std::vector<vec3> &colors);
        void update_normal_buffer(const std::vector<vec3> &normals);
        void update_texcoord_buffer(const std::vector<vec2> &texcoords);
        void update_index_buffer(const std::vector<unsigned int> &indices);

        /// selection buffer (internally based on a shader storage buffer)
        /// @param index: the index of the binding point.
        /// NOTE: the buffers should also be bound to this point in all shader code
        void update_selection_buffer(unsigned int index = 1);

        /// generic storage buffer
        /// @param index: the index of the binding point.
        /// NOTE: the buffers should also be bound to this point in all shader code
        void update_storage_buffer(const void *data, std::size_t datasize, unsigned int index = 0);

        /// Releases the index buffer if existing vertex data is sufficient (may require duplicating vertex data).
        void release_element_buffer();

        /**
         * @brief Requests an update of the OpenGL buffers.
         * @details This function sets the status to trigger an update of the OpenGL buffers. The actual update does not
         *          occur immediately but is deferred to the rendering phase.
         * @attention This method works for standard drawables (no update function required) and non-standard drawable
         *            (update function required). Standard drawables include:
         *              - SurfaceMesh: "faces", "edges", "vertices", "borders", "locks";
         *              - PointCloud: "vertices";
         *              - Graph: "edges", "vertices".
         * @TODO: promote face/vertex normals to the standard ones.
         */
        void update_buffers();

        /**
         * @brief Setups how a drwable can update its OpenGL buffers. This function is required by only non-standard
         *        drawables for a special visualization purpose. Standard drawables can be automatically updated and
         *        do not require this function.
         * @related renderer::update_buffers(...), update_buffers().
         */
        void set_update_func(std::function<void(Model*, Drawable*)> func) { update_func_ = func; }

        // ----------------- access data from the buffers -------------------

        void fetch_selection_buffer();

        // -------------------------- rendering -----------------------------

        State& state() { return *this; };
        const State& state() const { return *this; };
        void set_state(const State& s) { *((State*)this) = s; };

        /// The draw method.
        virtual void draw(const Camera *camera, bool with_storage_buffer = false) const = 0;

        /// The internal draw method of this drawable.
        /// NOTE: this functions should be called when your shader program is in use,
        ///		 i.e., between glUseProgram(id) and glUseProgram(0);
        void gl_draw(bool with_storage_buffer = false) const;

    protected:
        virtual void clear();
        virtual void internal_update_buffers();

        VertexArrayObject *vao() const { return vao_; }

    protected:
        std::string name_;
        Model *model_;
        Box3 bbox_;

        // vertex array object
        VertexArrayObject *vao_;

        std::size_t num_vertices_;
        std::size_t num_indices_;

        bool update_requested_;
        std::function<void(Model*, Drawable*)> update_func_;

        unsigned int vertex_buffer_;
        unsigned int color_buffer_;
        unsigned int normal_buffer_;
        unsigned int texcoord_buffer_;
        unsigned int index_buffer_;

        unsigned int storage_buffer_;
        std::size_t current_storage_buffer_size_;

        unsigned int selection_buffer_;  // used for selection.
        std::size_t current_selection_buffer_size_; // in case the object is modified
    };

}


#endif  // EASY3D_DRAWABLE_H
