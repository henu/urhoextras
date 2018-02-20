#ifndef URHOEXTRAS_MODELCOMBINER_HPP
#define URHOEXTRAS_MODELCOMBINER_HPP

#include <Urho3D/Container/Vector.h>
#include <Urho3D/Core/Mutex.h>
#include <Urho3D/Core/WorkQueue.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Math/Matrix4.h>
#include <Urho3D/Math/Quaternion.h>
#include <Urho3D/Math/Vector3.h>

namespace UrhoExtras
{

class ModelCombiner : public Urho3D::Object
{
	URHO3D_OBJECT(ModelCombiner, Urho3D::Object)

public:

	ModelCombiner(Urho3D::Context* context);
	virtual ~ModelCombiner();

	inline bool AddModel(Urho3D::Model const* model, Urho3D::Material* mat, Urho3D::Vector3 const& pos, Urho3D::Quaternion const& rot)
	{
		Urho3D::Matrix4 transf;
		transf.SetTranslation(pos);
		transf.SetRotation(rot.RotationMatrix());
		return AddModel(model, mat, transf);
	}

	inline bool AddModel(Urho3D::Model const* model, Urho3D::Material* mat, Urho3D::Matrix4 const& transf)
	{
		Urho3D::Vector<Urho3D::Material*> mats;
		mats.Reserve(model->GetNumGeometries());
		for (unsigned i = 0; i < model->GetNumGeometries(); ++ i) {
			mats.Push(mat);
		}
		return AddModel(model, mats, transf);
	}

	bool AddModel(Urho3D::Model const* model, Urho3D::Vector<Urho3D::Material*> const& mats, Urho3D::Matrix4 const& transf);

	// Functions to add Triangle
	bool StartAddingTriangle(Urho3D::PODVector<Urho3D::VertexElement> const& elems, Urho3D::Material* mat);
	inline bool AddTriangleData(float f) { return AddTriangleData((unsigned char const*)&f, sizeof(float)); }
	inline bool AddTriangleData(Urho3D::Vector2 const& v) { return AddTriangleData((unsigned char const*)v.Data(), sizeof(float) * 2); }
	inline bool AddTriangleData(Urho3D::Vector3 const& v) { return AddTriangleData((unsigned char const*)v.Data(), sizeof(float) * 3); }
	bool AddTriangleData(unsigned char const* buf, unsigned buf_size);

	// Check if combining is ready. This should
	// be called repeatedly until it returns true.
	bool Ready();

	// Blocks until ready.
	void FinalizeNow();

	Urho3D::Model* GetModel();
	Urho3D::Material* GetMaterial(unsigned geom_i);

private:

	typedef Urho3D::PODVector<unsigned char> ByteBuf;
	typedef Urho3D::PODVector<unsigned> IndexBuf;
	typedef Urho3D::HashMap<Urho3D::Material*, IndexBuf> IndexBufsByMaterial;
	typedef Urho3D::HashMap<unsigned, unsigned> IndexCache;

	struct RawVBuf
	{
		ByteBuf buf;
		unsigned vrt_size;
		Urho3D::PODVector<Urho3D::VertexElement> elems;
		IndexBufsByMaterial tris;
	};
	typedef Urho3D::Vector<RawVBuf> RawVBufs;

	struct QueueItem : Urho3D::RefCounted
	{
		unsigned vrt_size;
		ByteBuf vbuf;
		unsigned idx_size;
		ByteBuf ibuf;
		Urho3D::PODVector<Urho3D::VertexElement> elems;
		Urho3D::PrimitiveType primitive_type;
		Urho3D::Material* mat;
		Urho3D::Matrix4 transf;
	};
	typedef Urho3D::Vector<Urho3D::SharedPtr<QueueItem> > Queue;

	Queue queue;
	Urho3D::Mutex queue_mutex;

	Urho3D::SharedPtr<Urho3D::WorkItem> worker_wi;

	RawVBufs raw_vbufs;
	Urho3D::BoundingBox bb;

	// Triangle adding state
	Urho3D::PODVector<Urho3D::VertexElement> tri_add_elems;
	Urho3D::Material* tri_add_mat;
	ByteBuf tri_add_buf;
	unsigned tri_add_vrt_size;

	// State of process
	volatile bool no_more_input_coming;
	volatile bool give_up;

	// Results
	bool finalized;
	Urho3D::SharedPtr<Urho3D::Model> model;
	Urho3D::Vector<Urho3D::Material*> mats;

	RawVBuf* GetOrCreateVertexbuffer(unsigned vrt_size, Urho3D::PODVector<Urho3D::VertexElement> const& elems);

	int GetOrCreateVertexIndex(RawVBuf* raw_vbuf, unsigned char const* vrt_data, Urho3D::Matrix4 const& transf);

	inline static unsigned GetIndex(unsigned char const* ibuf, unsigned idx_size, unsigned idx)
	{
		if (idx_size == 1) return ibuf[idx];
		if (idx_size == 2) return ((unsigned short const*)ibuf)[idx];
		return ((unsigned const*)ibuf)[idx];
	}

	void AddTriangle(Urho3D::Material* mat, RawVBuf* raw_vbuf, unsigned vrt1, unsigned vrt2, unsigned vrt3);

	void MakeSureTaskIsRunning();

	static void Worker(Urho3D::WorkItem const* wi, unsigned thread_i);
};

}

#endif
