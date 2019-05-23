#include "modelcombiner.hpp"

#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/Graphics/IndexBuffer.h>
#include <Urho3D/Graphics/VertexBuffer.h>
#include <Urho3D/IO/Log.h>

namespace UrhoExtras
{

ModelCombiner::ModelCombiner(Urho3D::Context* context) :
Urho3D::Object(context),
tri_add_mat(NULL),
tri_add_vrt_size(0),
no_more_input_coming(false),
give_up(false),
finalized(false)
{
}

ModelCombiner::~ModelCombiner()
{
	give_up = true;
	if (worker_wi.NotNull()) {
		Urho3D::WorkQueue* workqueue = GetSubsystem<Urho3D::WorkQueue>();
		if (!workqueue->RemoveWorkItem(worker_wi)) {
			while (!worker_wi->completed_) {
			}
		}
	}
}

bool ModelCombiner::AddModel(Urho3D::Model const* model, Urho3D::Vector<Urho3D::Material*> const& mats, Urho3D::Matrix4 const& transf)
{
	if (no_more_input_coming) {
		URHO3D_LOGERROR("Unable to add models after using .ready()!");
		return false;
	}
	assert(mats.Size() == model->GetNumGeometries());
	for (unsigned geom_i = 0; geom_i < model->GetNumGeometries(); ++ geom_i) {
		Urho3D::Geometry const* geom = model->GetGeometry(geom_i, 0);
		Urho3D::Material* mat = mats[geom_i];

		// Get raw data from Geometry
		Urho3D::SharedPtr<QueueItem> qitem(new QueueItem);
		unsigned char const* vbuf;
		unsigned char const* ibuf;
		Urho3D::PODVector<Urho3D::VertexElement> const* elems;
		geom->GetRawData(vbuf, qitem->vrt_size, ibuf, qitem->idx_size, elems);
		// Find what is the biggest used vertex
		unsigned vbuf_size = 0;
		for (unsigned i = geom->GetVertexStart(); i < geom->GetIndexStart() + geom->GetIndexCount(); ++ i) {
			vbuf_size = Urho3D::Max<unsigned>(vbuf_size, GetIndex(ibuf, qitem->idx_size, i) + 1);
		}
		qitem->vbuf.Insert(qitem->vbuf.End(), vbuf, vbuf + vbuf_size * qitem->vrt_size);
		qitem->ibuf.Insert(qitem->ibuf.End(), ibuf + qitem->idx_size * geom->GetIndexStart(), ibuf + qitem->idx_size * (geom->GetIndexStart() + geom->GetIndexCount()));
		qitem->elems = *elems;
		qitem->primitive_type = geom->GetPrimitiveType();
		qitem->mat = mat;
		qitem->transf = transf;

		{
			Urho3D::MutexLock queue_lock(queue_mutex);
			(void)queue_lock;
			queue.Push(qitem);
			qitem = NULL;
		}

		MakeSureTaskIsRunning();
	}

	return true;
}

bool ModelCombiner::StartAddingTriangle(Urho3D::PODVector<Urho3D::VertexElement> const& elems, Urho3D::Material* mat)
{
	if (elems.Empty()) {
		URHO3D_LOGERROR("No elements!");
		return false;
	}
	if (tri_add_vrt_size) {
		URHO3D_LOGERROR("Unable to start adding new triangle because previous one is still incomplete!");
		return false;
	}

	tri_add_elems = elems;
	tri_add_mat = mat;
	tri_add_vrt_size = Urho3D::VertexBuffer::GetVertexSize(elems);
	assert(tri_add_buf.Empty());

	return true;
}

bool ModelCombiner::AddTriangleData(unsigned char const* buf, unsigned buf_size)
{
	if (tri_add_vrt_size == 0) {
		URHO3D_LOGERROR("No triangle adding started! Maybe previous triangle adding already got enough data?");
		return false;
	}
	tri_add_buf.Insert(tri_add_buf.End(), buf, buf + buf_size);
	// If too much data was got
	if (tri_add_buf.Size() > tri_add_vrt_size * 3) {
		URHO3D_LOGERROR("Too much data to when adding a triangle!");
		return false;
	}
	// If all data was got
	if (tri_add_buf.Size() == tri_add_vrt_size * 3) {
		// Convert triangle data to QueueItem.
		Urho3D::SharedPtr<QueueItem> qitem(new QueueItem);
		qitem->vrt_size = tri_add_vrt_size;
		qitem->vbuf.Swap(tri_add_buf);
		qitem->ibuf.Reserve(2*3);
		qitem->ibuf.Push(0); qitem->ibuf.Push(0);
		qitem->ibuf.Push(1); qitem->ibuf.Push(0);
		qitem->ibuf.Push(2); qitem->ibuf.Push(0);
		qitem->idx_size = 2;
		qitem->elems = tri_add_elems;
		qitem->primitive_type = Urho3D::TRIANGLE_LIST;
		qitem->mat = tri_add_mat;

		{
			Urho3D::MutexLock queue_lock(queue_mutex);
			(void)queue_lock;
			queue.Push(qitem);
			qitem = NULL;
		}

		MakeSureTaskIsRunning();

		tri_add_vrt_size = 0;
		assert(tri_add_buf.Empty());
	}
	return true;
}

bool ModelCombiner::Ready()
{
	// If already finalized
	if (finalized) {
		return true;
	}

	if (tri_add_vrt_size) {
		URHO3D_LOGERROR("Unable to finalize because there is an incomplete triangle adding!");
		return false;
	}

	// Mark that no other input is coming
	no_more_input_coming = true;

	// Check if there is still unprocessed stuff in queue
	{
		Urho3D::MutexLock queue_lock(queue_mutex);
		(void)queue_lock;
		if (!queue.Empty()) {
			// There is still stuff to process, make
			// sure task is running and try again later.
			MakeSureTaskIsRunning();
			return false;
		}
	}
	// Queue is empty, but worker unit needs to be waited too.
	if (worker_wi && !worker_wi->completed_) {
		return false;
	}

	// Discard previous possible incomplete results, just to be sure
	mats.Clear();

	Urho3D::Vector<Urho3D::SharedPtr<Urho3D::IndexBuffer> > ibufs;
	Urho3D::Vector<Urho3D::SharedPtr<Urho3D::VertexBuffer> > vbufs;
	Urho3D::Vector<Urho3D::SharedPtr<Urho3D::Geometry> > geoms;
	for (RawVBuf const& raw_vbuf : raw_vbufs) {
		// Create Vertexbuffer
		Urho3D::SharedPtr<Urho3D::VertexBuffer> vbuf(new Urho3D::VertexBuffer(context_));
		vbuf->SetShadowed(true);
		assert(raw_vbuf.buf.Size() % raw_vbuf.vrt_size == 0);
		if (!vbuf->SetSize(raw_vbuf.buf.Size() / raw_vbuf.vrt_size, raw_vbuf.elems)) {
			URHO3D_LOGERROR("Unable to set vertexbuffer size!");
			return false;
		}
		if (!vbuf->SetData(raw_vbuf.buf.Buffer())) {
			URHO3D_LOGERROR("Unable to set vertexbuffer data!");
			return false;
		}
		vbufs.Push(vbuf);

		// Create single indexbuffer
		ByteBuf total_ibuf_bytes;
		unsigned idx_size;
		if (vbuf->GetVertexCount() <= 65536) {
			idx_size = 2;
			total_ibuf_bytes.Reserve(2 * vbuf->GetVertexCount());
			for (IndexBufsByMaterial::ConstIterator tris_i = raw_vbuf.tris.Begin(); tris_i != raw_vbuf.tris.End(); ++ tris_i) {
				for (unsigned i : tris_i->second_) {
					unsigned short i2 = i;
					total_ibuf_bytes.Insert(total_ibuf_bytes.End(), (unsigned char*)&i2, (unsigned char*)&i2 + 2);
				}
			}
		} else {
			idx_size = 4;
			total_ibuf_bytes.Reserve(4 * vbuf->GetVertexCount());
			for (IndexBufsByMaterial::ConstIterator tris_i = raw_vbuf.tris.Begin(); tris_i != raw_vbuf.tris.End(); ++ tris_i) {
				for (unsigned i : tris_i->second_) {
					total_ibuf_bytes.Insert(total_ibuf_bytes.End(), (unsigned char*)&i, (unsigned char*)&i + 4);
				}
			}
		}
		Urho3D::SharedPtr<Urho3D::IndexBuffer> ibuf(new Urho3D::IndexBuffer(context_));
		ibuf->SetShadowed(true);
		if (!ibuf->SetSize(total_ibuf_bytes.Size() / idx_size, idx_size == 4)) {
			URHO3D_LOGERROR("Unable to set indexbuffer size!");
			return false;
		}
		if (!ibuf->SetData(total_ibuf_bytes.Buffer())) {
			URHO3D_LOGERROR("Unable to set indexbuffer data!");
			return false;
		}
		ibufs.Push(ibuf);

		// Create geometries
		unsigned ibuf_ofs = 0;
		for (IndexBufsByMaterial::ConstIterator tris_i = raw_vbuf.tris.Begin(); tris_i != raw_vbuf.tris.End(); ++ tris_i) {
			Urho3D::Material* mat = tris_i->first_;
			IndexBuf const& ibuf_raw = tris_i->second_;

			Urho3D::SharedPtr<Urho3D::Geometry> geom(new Urho3D::Geometry(context_));
			if (!geom->SetNumVertexBuffers(1)) {
				URHO3D_LOGERROR("Unable to set number of vertexbuffers in geometry!");
				return false;
			}
			if (!geom->SetVertexBuffer(0, vbuf)) {
				URHO3D_LOGERROR("Unable to set geometry vertexbuffer!");
				return false;
			}
			geom->SetIndexBuffer(ibuf);
			if (!geom->SetDrawRange(Urho3D::TRIANGLE_LIST, ibuf_ofs, ibuf_raw.Size())) {
				URHO3D_LOGERROR("Unable to set geometry drawing range!");
				return false;
			}

			geoms.Push(geom);
			mats.Push(mat);

			ibuf_ofs += ibuf_raw.Size();
		}
	}

	// If this would result to empty model, then stop here
	if (geoms.Empty()) {
		finalized = true;
		return true;
	}

	// Create and set up Model
	model = new Urho3D::Model(context_);
	if (model.Null()) {
		URHO3D_LOGERROR("Unable to create model!");
		return false;
	}
	if (!model->SetIndexBuffers(ibufs)) {
		URHO3D_LOGERROR("Unable to set indexbuffers of model!");
		return false;
	}
	if (!model->SetVertexBuffers(vbufs, Urho3D::PODVector<unsigned>(), Urho3D::PODVector<unsigned>())) {
		URHO3D_LOGERROR("Unable to set vertexbuffers of model!");
		return false;
	}
	model->SetNumGeometries(geoms.Size());
	for (unsigned geom_i = 0; geom_i < geoms.Size(); ++ geom_i) {
		if (!model->SetGeometry(geom_i, 0, geoms[geom_i])) {
			URHO3D_LOGERROR("Unable to set geometry of model!");
			return false;
		}
	}
	model->SetBoundingBox(bb);

	// Clean temporary data
	raw_vbufs.Clear();

	finalized = true;

	return true;
}

void ModelCombiner::FinalizeNow()
{
// TODO: Ask thread to stop and finalize in this thread!
while (!Ready()) {
}
}

Urho3D::Model* ModelCombiner::GetModel()
{
	if (!finalized) {
		URHO3D_LOGERROR("Model combining must be finalized first!");
		return NULL;
	}
	return model;
}

Urho3D::Material* ModelCombiner::GetMaterial(unsigned geom_i)
{
	if (!finalized) {
		URHO3D_LOGERROR("Model combining must be finalized first!");
		return NULL;
	}
	assert(geom_i < model->GetNumGeometries());
	return mats[geom_i];
}

ModelCombiner::RawVBuf* ModelCombiner::GetOrCreateVertexbuffer(unsigned vrt_size, Urho3D::PODVector<Urho3D::VertexElement> const& elems)
{
	for (RawVBuf& raw_vbuf : raw_vbufs) {
		if (raw_vbuf.vrt_size == vrt_size && raw_vbuf.elems == elems) {
			return &raw_vbuf;
		}
	}
	RawVBuf new_raw_vbuf;
	new_raw_vbuf.vrt_size = vrt_size;
	new_raw_vbuf.elems = elems;
	raw_vbufs.Push(new_raw_vbuf);
	return &raw_vbufs.Back();
}

int ModelCombiner::GetOrCreateVertexIndex(RawVBuf* raw_vbuf, unsigned char const* vrt_data, Urho3D::Matrix4 const& transf)
{
	// Apply transform to vertex data
	ByteBuf vrt_data_transfd;
	for (Urho3D::VertexElement const& elem : raw_vbuf->elems) {
		if (elem.semantic_ == Urho3D::SEM_POSITION) {
			if (elem.type_ == Urho3D::TYPE_VECTOR3) {
				Urho3D::Vector3 pos((float*)(vrt_data + elem.offset_));
				pos = transf * pos;
				vrt_data_transfd.Insert(vrt_data_transfd.End(), (unsigned char*)pos.Data(), (unsigned char*)pos.Data() + sizeof(float) * 3);
				bb.Merge(pos);
			} else {
				URHO3D_LOGERROR("For SEM_POSITION only TYPE_VECTOR3 is supported for now!");
				return -1;
			}
		} else if (elem.semantic_ == Urho3D::SEM_NORMAL || elem.semantic_ == Urho3D::SEM_BINORMAL || elem.semantic_ == Urho3D::SEM_TANGENT) {
			if (elem.type_ == Urho3D::TYPE_VECTOR3) {
				Urho3D::Vector3 vec((float*)(vrt_data + elem.offset_));
				vec = transf.RotationMatrix() * vec;
				vrt_data_transfd.Insert(vrt_data_transfd.End(), (unsigned char*)vec.Data(), (unsigned char*)vec.Data() + sizeof(float) * 3);
			} else {
				URHO3D_LOGERROR("For SEM_NORMAL, SEM_BINORMAL and SEM_TANGENT only TYPE_VECTOR3 is supported for now!");
				return -1;
			}
		} else {
			unsigned elem_size;
			switch (elem.type_) {
			case Urho3D::TYPE_INT:
				elem_size = sizeof(int);
				break;
			case Urho3D::TYPE_FLOAT:
				elem_size = sizeof(float);
				break;
			case Urho3D::TYPE_VECTOR2:
				elem_size = sizeof(float) * 2;
				break;
			case Urho3D::TYPE_VECTOR3:
				elem_size = sizeof(float) * 3;
				break;
			case Urho3D::TYPE_VECTOR4:
				elem_size = sizeof(float) * 4;
				break;
			default:
				URHO3D_LOGERRORF("Unsupported element type(%i)!", elem.type_);
				return -1;
			}
			vrt_data_transfd.Insert(vrt_data_transfd.End(), vrt_data + elem.offset_, vrt_data + elem.offset_ + elem_size);
		}
	}

	// Go existing vertices through, and try to find a match
	for (unsigned i = 0; i < raw_vbuf->buf.Size(); i += raw_vbuf->vrt_size) {
		bool all_match = true;
		for (Urho3D::VertexElement const& elem : raw_vbuf->elems) {
			unsigned char const* ptr1 = raw_vbuf->buf.Buffer() + i + elem.offset_;
			unsigned char const* ptr2 = vrt_data_transfd.Buffer() + elem.offset_;
			switch (elem.type_) {
			case Urho3D::TYPE_INT:
			{
				int i1 = *(int*)ptr1;
				int i2 = *(int*)ptr2;
				if (i1 != i2) all_match = false;
				break;
			}
			case Urho3D::TYPE_FLOAT:
			{
				float f1 = *(float*)ptr1;
				float f2 = *(float*)ptr2;
				if (fabs(f1 - f2) > Urho3D::M_EPSILON) all_match = false;
				break;
			}
			case Urho3D::TYPE_VECTOR2:
			{
				Urho3D::Vector2 v1((float*)ptr1);
				Urho3D::Vector2 v2((float*)ptr2);
				if ((v1 - v2).Length() > Urho3D::M_EPSILON) all_match = false;
				break;
			}
			case Urho3D::TYPE_VECTOR3:
			{
				Urho3D::Vector3 v1((float*)ptr1);
				Urho3D::Vector3 v2((float*)ptr2);
				if ((v1 - v2).Length() > Urho3D::M_EPSILON) all_match = false;
				break;
			}
			case Urho3D::TYPE_VECTOR4:
			{
				Urho3D::Vector4 v1((float*)ptr1);
				Urho3D::Vector4 v2((float*)ptr2);
				Urho3D::Vector4 diff = v1 - v2;
				float len = sqrt(diff.x_*diff.x_ + diff.y_*diff.y_ + diff.z_*diff.z_ + diff.w_*diff.w_);
				if (len > Urho3D::M_EPSILON) all_match = false;
				break;
			}
			default:
				URHO3D_LOGERRORF("Unsupported element type(%i)!", elem.type_);
				return -1;
			}
			if (!all_match) {
				break;
			}
		}
		if (all_match) {
			assert(i % raw_vbuf->vrt_size == 0);
			return i / raw_vbuf->vrt_size;
		}
	}

	// No matching vertex was found, so new one needs to be created
	assert(raw_vbuf->buf.Size() % raw_vbuf->vrt_size == 0);
	unsigned result = raw_vbuf->buf.Size() / raw_vbuf->vrt_size;
	raw_vbuf->buf.Insert(raw_vbuf->buf.End(), vrt_data_transfd.Begin(), vrt_data_transfd.End());
	return result;
}

void ModelCombiner::AddTriangle(Urho3D::Material* mat, RawVBuf* raw_vbuf, unsigned vrt1, unsigned vrt2, unsigned vrt3)
{
	IndexBufsByMaterial::Iterator tris_find = raw_vbuf->tris.Find(mat);
	if (tris_find == raw_vbuf->tris.End()) {
		tris_find = raw_vbuf->tris.Insert(Urho3D::Pair<Urho3D::Material*, IndexBuf>(mat, IndexBuf()));
	}
	IndexBuf& ibuf = tris_find->second_;
	ibuf.Push(vrt1);
	ibuf.Push(vrt2);
	ibuf.Push(vrt3);
}

void ModelCombiner::MakeSureTaskIsRunning()
{
	if (worker_wi.Null() || worker_wi->completed_) {
		worker_wi = new Urho3D::WorkItem();
		worker_wi->workFunction_ = Worker;
		worker_wi->aux_ = this;
		Urho3D::WorkQueue* workqueue = GetSubsystem<Urho3D::WorkQueue>();
		workqueue->AddWorkItem(worker_wi);
	}
}

void ModelCombiner::Worker(Urho3D::WorkItem const* wi, unsigned thread_i)
{
	(void)thread_i;
	ModelCombiner* combiner = (ModelCombiner*)wi->aux_;

	// Loop as long as there is stuff in the
	// queue or until giving up is requested.
	while (!combiner->give_up) {

		// Pick one item from the queue,
		// or leave if queue is empty.
		Urho3D::SharedPtr<QueueItem> qitem;
		{
			Urho3D::MutexLock queue_lock(combiner->queue_mutex);
			(void)queue_lock;
			if (combiner->queue.Empty()) {
				return;
			}
			qitem = combiner->queue.Back();
			combiner->queue.Pop();
		}

		RawVBuf* raw_vbuf = combiner->GetOrCreateVertexbuffer(qitem->vrt_size, qitem->elems);

		if (qitem->primitive_type == Urho3D::TRIANGLE_LIST) {
			IndexCache index_cache;
			unsigned index_end = 0 + qitem->ibuf.Size() / qitem->idx_size;
			for (unsigned i = 0; i < index_end; i += 3) {
				// Check if worker should give up
				if (combiner->give_up) {
					return;
				}
				// Get indices in source model
				int vrt_i1 = GetIndex(qitem->ibuf.Buffer(), qitem->idx_size, i);
				int vrt_i2 = GetIndex(qitem->ibuf.Buffer(), qitem->idx_size, i + 1);
				int vrt_i3 = GetIndex(qitem->ibuf.Buffer(), qitem->idx_size, i + 2);
				// Convert to indices in target model. Use cache to speed up
				IndexCache::ConstIterator index_cache_find = index_cache.Find(vrt_i1);
				if (index_cache_find != index_cache.End()) vrt_i1 = index_cache_find->second_;
				else vrt_i1 = index_cache[vrt_i1] = combiner->GetOrCreateVertexIndex(raw_vbuf, qitem->vbuf.Buffer() + qitem->vrt_size * vrt_i1, qitem->transf);
				index_cache_find = index_cache.Find(vrt_i2);
				if (index_cache_find != index_cache.End()) vrt_i2 = index_cache_find->second_;
				else vrt_i2 = index_cache[vrt_i2] = combiner->GetOrCreateVertexIndex(raw_vbuf, qitem->vbuf.Buffer() + qitem->vrt_size * vrt_i2, qitem->transf);
				index_cache_find = index_cache.Find(vrt_i3);
				if (index_cache_find != index_cache.End()) vrt_i3 = index_cache_find->second_;
				else vrt_i3 = index_cache[vrt_i3] = combiner->GetOrCreateVertexIndex(raw_vbuf, qitem->vbuf.Buffer() + qitem->vrt_size * vrt_i3, qitem->transf);
				// If there was errors.
				if (vrt_i1 < 0 || vrt_i2 < 0 || vrt_i3 < 0) {
					return;
				}
				combiner->AddTriangle(qitem->mat, raw_vbuf, vrt_i1, vrt_i2, vrt_i3);
			}
		} else {
			URHO3D_LOGERROR("ModelCombiner only supports TRIANGLE_LIST for now!");
			return;
		}
	}
}

}
