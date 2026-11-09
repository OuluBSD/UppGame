#include "SweetSandy.h"

NAMESPACE_UPP

namespace {

// The two clips this fork's ogre ships, and the walking speed that switches
// between them. `packages/models/ogro2/iqm.cfg` names them `idle` and
// `run.iqm` (`iqmanim "forward" "run.iqm"`), and the file's own internal
// animation name is not used - the spec appended by `AppendAnims` is taken as
// it comes, so a renamed file still works.
const char *const kIdleFile = "idle";
const char *const kRunFile = "run";
// The skin texture `iqm.cfg` binds to every part of the model.
const char *const kSkinFile = "green.jpg";

// Below this walking speed (m/s) the character idles in place instead of
// running. SweetSandy's pedestrians walk at 0.8-1.5 m/s, so this only ever
// separates "standing still" from "walking" - it is not a walk/run blend, and
// pretending otherwise would be a claim the two clips cannot support.
const double kRunSpeedThreshold = 0.05;

// The model's forward axis, as a yaw correction applied before the
// pedestrian's own heading. The ogre's feet and lower legs are elongated
// along z in the bind pose (sd 0.65 along z against 0.28 along x over the
// lowest fifth of the model) and the tail of that distribution - the toes -
// is at +z, so +z is the way it looks. Rt3D's IQM conversion negates y only,
// which is why this is 0 and not 180.
const double kModelYawCorrection = 0.0;

} // anonymous namespace

String PedestrianModel::FindModelFile(const String& name)
{
	// The same search order the Rt3D self-test uses, so one checkout serves
	// both: an explicit override first, then the paths relative to a
	// Tesseract checkout that may be anywhere.
	Vector<String> dirs;
	String dir = GetEnv("RT3_MODEL_DIR");
	if(!dir.IsEmpty()) dirs.Add(dir);
	dirs.Add("packages/models/ogro2");
	dirs.Add("Tesseract-Sauerbraten/packages/models/ogro2");
	String home = GetEnv("HOME");
	if(home.IsEmpty()) home = GetEnv("USERPROFILE");
	if(!home.IsEmpty())
	{
		dirs.Add(home + "/Dev/Tesseract-Sauerbraten/packages/models/ogro2");
		dirs.Add(home + "/dev-linux/Tesseract-Sauerbraten/packages/models/ogro2");
	}
	for(int i = 0; i < dirs.GetCount(); i++)
	{
		String path = dirs[i] + "/" + name + ".iqm";
		if(FileExists(path)) return path;
	}
	return Null;
}

bool PedestrianModel::Load(double target_height, String& error)
{
	error = Null;
	loaded = false;
	String meshpath = FindModelFile("ogro");
	if(meshpath.IsEmpty())
	{
		error = "no ogro.iqm found (set RT3_MODEL_DIR or run next to a Tesseract checkout)";
		return false;
	}
	String folder = GetFileFolder(meshpath);
	model_folder = folder;

	// ---- the mesh -----------------------------------------------------------
	Rt3IqmModel mdl;
	if(!Rt3IqmModel::Load(meshpath, mdl, error)) return false;
	if(!mdl.BuildSkeleton(model.skel, error)) return false;
	if(!mdl.BuildMesh(model.skel, error)) return false;
	mdl.SortBlendCombos(model.skel);
	model.skel.RemapBones();
	model.skel.PrepCpuSkinning();
	mdl.RemapBlendVertices(model.skel);
	mdl.BuildSkelMeshes(model.skel, model.meshes);
	if(model.meshes.IsEmpty())
	{
		error = "the ogre file has no usable mesh";
		return false;
	}
	// One anim part, and it drives every bone.
	partmask.SetCount(model.skel.numbones);
	for(int i = 0; i < partmask.GetCount(); i++) partmask[i] = 0;

	// ---- the clips ----------------------------------------------------------
	// Each file is loaded and appended on its own, and the spec it appends is
	// taken from the skeleton's tail: the file's internal animation name is
	// Tesseract's business, the frame range is all this needs.
	struct { const char *file; PedestrianClip *clip; } clips[2] = {
		{kIdleFile, &idle}, {kRunFile, &run}
	};
	for(int i = 0; i < 2; i++)
	{
		String path = FindModelFile(clips[i].file);
		if(path.IsEmpty()) continue;
		Rt3IqmModel anim;
		if(!Rt3IqmModel::Load(path, anim, error)) return false;
		int before = model.skel.skelanims.GetCount();
		if(!anim.AppendAnims(model.skel, path, error)) return false;
		if(model.skel.skelanims.GetCount() <= before) continue;
		const Rt3SkelAnimSpec& spec = model.skel.skelanims.Top();
		clips[i].clip->label = clips[i].file;
		clips[i].clip->frame = spec.frame;
		clips[i].clip->range = spec.range;
		// The exporter's own framerate is milliseconds per frame; a file
		// without one plays at ten frames a second, which is what this fork's
		// clips are.
		clips[i].clip->speed = anim.anims.IsEmpty() || anim.anims[0].framerate <= 0
		                       ? 100.0 : 1000.0 / anim.anims[0].framerate;
	}
	if(!idle.IsValid() && !run.IsValid())
	{
		error = "neither idle.iqm nor run.iqm could be loaded - the character would stand still";
		return false;
	}

	// ---- placement ----------------------------------------------------------
	// The model is scaled so that its own bind-pose height is `target_height`
	// meters, and lifted so its lowest bind-pose vertex stands on the ground.
	// Both come out of the bind pose rather than out of a constant, because
	// the ogre's proportions (2.25 units tall, 4.18 across including the
	// weapon) are not something to hardcode a second copy of.
	double lo = 0, hi = 0;
	bool first = true;
	for(int m = 0; m < model.meshes.GetCount(); m++)
	{
		const Vector<Rt3SkelVert>& verts = model.meshes[m].verts;
		for(int i = 0; i < verts.GetCount(); i++)
		{
			double y = verts[i].pos.y;
			if(first) { lo = hi = y; first = false; }
			else { lo = min(lo, y); hi = max(hi, y); }
		}
	}
	if(!(hi > lo))
	{
		error = "the ogre's bind pose has no height";
		return false;
	}
	scale = target_height / (hi - lo);
	ground_offset = -lo * scale;
	model_height = target_height;

	model.GenVbo();
	if(model.vlen <= 0 || model.elems.IsEmpty())
	{
		error = "the ogre laid out to an empty mesh";
		return false;
	}
	loaded = true;
	LOG("PedestrianModel: ogre " << model.vlen << " verts, " << model.elems.GetCount() / 3
	    << " tris, " << model.skel.numbones << " bones, idle " << idle.range
	    << " frames, run " << run.range << " frames, height " << Format("%.2f", model_height)
	    << "m (scale " << Format("%.4f", scale) << ") from " << meshpath);
	return true;
}

PedestrianClip PedestrianModel::ClipForSpeed(double speed, double run_threshold) const
{
	const PedestrianClip *c = speed > run_threshold ? &run : &idle;
	if(!c->IsValid()) c = c == &run ? &idle : &run;
	return *c;
}

bool PedestrianModel::Evaluate(const String& clip, dword millis, Vec3& bbmin, Vec3& bbmax)
{
	if(!loaded) return false;
	PedestrianClip c = clip == "run" ? run : idle;
	if(!c.IsValid()) return false;

	Rt3AnimInfo info;
	info.anim = RT3_ANIM_LOOP;
	info.frame = c.frame;
	info.range = c.range;
	info.speed = c.speed;
	Rt3AnimState as;
	as.cur.SetFrames(info, millis);
	as.interp = 1;
	model.Animate(&as, cache, 0, Rt3Vec3(0, 0, 1), Rt3Vec3(1, 0, 0), 1, partmask.Begin());

	// In **meters**, the same units `GetHeight` and the collider use: the model
	// is loaded at a scale that makes its own height a real pedestrian's, and a
	// bounding box in model units next to a height in meters would compare two
	// different things.
	bool first = true;
	for(int i = 0; i < model.vlen; i++)
	{
		const Rt3Vec3& p = model.vdata[i].pos;
		Vec3 v(p.x * scale, p.y * scale, p.z * scale);
		if(first) { bbmin = bbmax = v; first = false; }
		else
		{
			bbmin.x = min(bbmin.x, v.x); bbmin.y = min(bbmin.y, v.y); bbmin.z = min(bbmin.z, v.z);
			bbmax.x = max(bbmax.x, v.x); bbmax.y = max(bbmax.y, v.y); bbmax.z = max(bbmax.z, v.z);
		}
	}
	last_skinned = model.vlen;
	return true;
}

void PedestrianModel::DrawGL(Vec3 pos, double ground_y, double yaw, double speed,
                             dword anim_ms, dword phase_offset)
{
	if(!loaded) return;
	PedestrianClip c = ClipForSpeed(speed, kRunSpeedThreshold);
	if(!c.IsValid()) return;

	// The run clip is authored at a run; a pedestrian walking at 0.8 m/s plays
	// it slower rather than in a different clip, because that is the only
	// honest way to get a slower stride out of these two files. The idle clip
	// has no such scaling - it is a standing loop.
	double rate = 1.0;
	if(c.label == kRunFile)
	{
		rate = speed / 1.5;   // SweetSandy's own fastest walking pace
		rate = clamp(rate, 0.35, 1.0);
	}

	Rt3AnimInfo info;
	info.anim = RT3_ANIM_LOOP;
	info.frame = c.frame;
	info.range = c.range;
	info.speed = c.speed / rate;
	Rt3AnimState as;
	as.cur.SetFrames(info, (dword)(anim_ms + phase_offset));
	as.interp = 1;
	model.Animate(&as, cache, 0, Rt3Vec3(0, 0, 1), Rt3Vec3(1, 0, 0), 1, partmask.Begin());
	last_skinned = model.vlen;

	if(!skin_tex)
	{
		// Lazy, because this is the first thing in the class that needs a GL
		// context, and the self-test never has one.
		skin_tex = model_folder.IsEmpty() ? 0 : LoadGLTexture(model_folder + "/" + kSkinFile);
		if(!skin_tex)
			LOG("PedestrianModel: no skin texture, drawing the ogre untextured");
	}

	// The character's own frame: standing on the ground at the pedestrian's
	// position, facing the pedestrian's heading, at the model's scale, with the
	// model-space y axis up. SweetSandy is +y up and +z south, and Rt3D's
	// model space is already +y up, so only the yaw and the two offsets are
	// needed.
	glPushMatrix();
	glTranslated(pos.x, ground_y, pos.y);
	glRotated(-yaw * 180.0 / M_PI + kModelYawCorrection, 0, 1, 0);
	glScaled(scale, scale, scale);
	glTranslated(0, ground_offset, 0);

	glColor3ub(255, 255, 255);
	// Two-sided lighting, and this is not a stylistic choice: the ogre's
	// triangles come out of the IQM y-flip with their winding reversed, so
	// from outside the model they are *back* faces - and a back face with GL's
	// one-sided lighting is black however bright the light is. Tesseract's own
	// model shaders do the equivalent per fragment (`if(!gl_FrontFacing) N =
	// -N`); there is no shader here, so this is the fixed-function version of
	// the same fix. Without it the character draws as a correct black
	// silhouette, which is exactly what the first capture showed.
	GLboolean two_side = glIsEnabled(GL_LIGHT_MODEL_TWO_SIDE);
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
	if(skin_tex)
	{
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, skin_tex);
	}
	// Lit and textured, unlike the flat figure this replaces - the skinned
	// normals are real, so this is the same treatment DrawVegetationGL gives
	// its own meshes. GL_LIGHTING is on here because GLPaint leaves it on.
	glBegin(GL_TRIANGLES);
	for(int i = 0; i < model.elems.GetCount(); i++)
	{
		const Rt3SkelVert& v = model.vdata[model.elems[i]];
		glNormal3d(v.norm.x, v.norm.y, v.norm.z);
		glTexCoord2d(v.u, v.v);
		glVertex3d(v.pos.x, v.pos.y, v.pos.z);
	}
	glEnd();
	if(skin_tex)
	{
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);
	}
	glPopMatrix();
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, two_side ? GL_TRUE : GL_FALSE);
}

END_UPP_NAMESPACE
