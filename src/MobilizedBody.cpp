/* Portions copyright (c) 2007 Stanford University and Michael Sherman.
 * Contributors:
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including 
 * without limitation the rights to use, copy, modify, merge, publish, 
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included 
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS, CONTRIBUTORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**@file
 *
 * Private implementation of MobilizedBody, and its included subclasses which
 * represent the built-in mobilizer types.
 */

#include "SimTKsimbody.h"
#include "simbody/internal/MobilizedBody.h"

#include "MobilizedBodyRep.h"
#include "MatterSubsystemRep.h"
#include "SimbodyMatterSubsystemRep.h"

namespace SimTK {

    //////////
    // BODY //
    //////////

bool Body::isEmptyHandle() const {return rep==0;}
bool Body::isOwnerHandle() const {return rep==0 || rep->myHandle==this;}

Body::~Body() {
    if (isOwnerHandle()) delete rep; 
    rep=0;
}

// Copy constructor creates a new copy of the source object.
Body::Body(const Body& src) : rep(0) {
    if (src.rep) {
        rep = src.rep->clone();
        rep->setMyHandle(*this);
    }
}

// Assignment puts a copy of the src Body's rep into the current handle.
Body& Body::operator=(const Body& src) {
    if (&src != this) {
        if (isOwnerHandle()) delete rep; 
        rep=0;
        if (src.rep) {
			rep = src.rep->clone();	// create a new object
			rep->setMyHandle(*this);
        }
    }
    return *this;
}

const MassProperties& Body::getDefaultRigidBodyMassProperties() const {
    return getRep().getDefaultRigidBodyMassProperties();
}


    /////////////////
    // BODY::RIGID //
    /////////////////

Body::Rigid::Rigid() {
    rep = new RigidRep();
    rep->setMyHandle(*this);
}

Body::Rigid::Rigid(const MassProperties& m) {
    rep = new RigidRep(m);
    rep->setMyHandle(*this);
}

Body::Rigid& Body::Rigid::setDefaultMassProperties(const MassProperties& m) {
    updRep().setDefaultMassProperties(m);
    return *this;
}

bool Body::Rigid::isInstanceOf(const Body& b) {
    return RigidRep::isA(b.getRep());
}
const Body::Rigid& Body::Rigid::downcast(const Body& b) {
    assert(isInstanceOf(b));
    return reinterpret_cast<const Rigid&>(b);
}
Body::Rigid& Body::Rigid::updDowncast(Body& b) {
    assert(isInstanceOf(b));
    return reinterpret_cast<Rigid&>(b);
}
const Body::Rigid::RigidRep& Body::Rigid::getRep() const {
    return dynamic_cast<const RigidRep&>(*rep);
}
Body::Rigid::RigidRep& Body::Rigid::updRep() {
    return dynamic_cast<RigidRep&>(*rep);
}

    //////////////////
    // BODY::GROUND //
    //////////////////

Body::Ground::Ground() {
    rep = new GroundRep();
    rep->setMyHandle(*this);
}

bool Body::Ground::isInstanceOf(const Body& b) {
    return GroundRep::isA(b.getRep());
}
const Body::Ground& Body::Ground::downcast(const Body& b) {
    assert(isInstanceOf(b));
    return reinterpret_cast<const Ground&>(b);
}
Body::Ground& Body::Ground::updDowncast(Body& b) {
    assert(isInstanceOf(b));
    return reinterpret_cast<Ground&>(b);
}
const Body::Ground::GroundRep& Body::Ground::getRep() const {
    return dynamic_cast<const GroundRep&>(*rep);
}
Body::Ground::GroundRep& Body::Ground::updRep() {
    return dynamic_cast<GroundRep&>(*rep);
}

    ////////////////////
    // MOBILIZED BODY //
    ////////////////////

bool MobilizedBody::isEmptyHandle() const {return rep==0;}
bool MobilizedBody::isOwnerHandle() const {return rep==0 || rep->myHandle==this;}

void MobilizedBody::disown(MobilizedBody& newOwnerHandle) {
    SimTK_ASSERT_ALWAYS(rep && rep->myHandle==this,
        "disown() not allowed for an empty or non-owner MobilizedBody handle.");
    SimTK_ASSERT_ALWAYS(!newOwnerHandle.rep,
        "disown() can only transfer ownership to an empty MobilizedBody handle.");

    newOwnerHandle.setRep(*rep);
    rep->setMyHandle(newOwnerHandle);
}

MobilizedBody::~MobilizedBody() {
    if (isOwnerHandle()) delete rep; 
    rep=0;
}

// Make this MobilizedBody a non-owner handle referring to the same
// object as the source.
MobilizedBody::MobilizedBody(MobilizedBody& src) : rep(src.rep) {
}

// Make this empty or non-owner handle refer to the same object
// as the source. This is illegal if the current handle is an
// owner.
MobilizedBody& MobilizedBody::operator=(MobilizedBody& src) {
    if (&src != this) {
        SimTK_ASSERT_ALWAYS(!(rep && rep->myHandle==this),
            "You can't reassign the owner handle of a MobilizedBody.");
        rep = src.rep;
    }
    return *this;
}

const SimbodyMatterSubsystem& MobilizedBody::getMatterSubsystem() const {
    SimTK_ASSERT_ALWAYS(isInSubsystem(),
        "getMatterSubsystem() called on a MobilizedBody that is not part of a subsystem.");
    return rep->getMyMatterSubsystemRep().getMySimbodyMatterSubsystemHandle();
}


MobilizedBodyId MobilizedBody::getMobilizedBodyId() const {
    SimTK_ASSERT_ALWAYS(isInSubsystem(),
        "getMobilizedBodyId() called on a MobilizedBody that is not part of a subsystem.");
    return rep->myMobilizedBodyId;
}

const MobilizedBody& MobilizedBody::getInboardMobilizedBody() const {
    SimTK_ASSERT_ALWAYS(isInSubsystem(),
        "getInboardMobilizedBody() called on a MobilizedBody that is not part of a subsystem.");
    return rep->getMyMatterSubsystemRep().getMobilizedBody(rep->myParentId);
}

bool MobilizedBody::isInSubsystem() const {
    return rep && rep->isInSubsystem();
}

bool MobilizedBody::isInSameSubsystem(const MobilizedBody& otherBody) const {
    return isInSubsystem() && otherBody.isInSubsystem()
           && getMatterSubsystem().isSameSubsystem(otherBody.getMatterSubsystem());
}


SimbodyMatterSubsystem& MobilizedBody::updMatterSubsystem() {
    SimTK_ASSERT_ALWAYS(isInSubsystem(),
        "updMatterSubsystem() called on a MobilizedBody that is not part of a subsystem.");
    return rep->updMyMatterSubsystemRep().updMySimbodyMatterSubsystemHandle();
}

const Body& MobilizedBody::getBody() const {
    return getRep().theBody;
}
Body& MobilizedBody::updBody() {
    if (getRep().isInSubsystem())
        updRep().updMyMatterSubsystemRep().invalidateSubsystemTopologyCache();
    return updRep().theBody;
}
MobilizedBody& MobilizedBody::setBody(const Body& b) {
    updBody() = b;
    return *this;
}
MobilizedBody& MobilizedBody::setDefaultInboardFrame (const Transform& X_PMb) {
    updRep().defaultInboardFrame = X_PMb;
    if (getRep().isInSubsystem())
        updRep().updMyMatterSubsystemRep().invalidateSubsystemTopologyCache();
    return *this;
}
MobilizedBody& MobilizedBody::setDefaultOutboardFrame(const Transform& X_BM) {
    updRep().defaultOutboardFrame = X_BM;
    if (getRep().isInSubsystem())
        updRep().updMyMatterSubsystemRep().invalidateSubsystemTopologyCache();
    return *this;
}

const Transform& MobilizedBody::getDefaultInboardFrame() const {
    return getRep().defaultInboardFrame;
}
const Transform& MobilizedBody::getDefaultOutboardFrame() const {
    return getRep().defaultOutboardFrame;
}

// Access to State

const Transform& MobilizedBody::getBodyTransform(const State& s) const {
    return getRep().getBodyTransform(s);
}
const Transform& MobilizedBody::getMobilizerTransform(const State& s) const {
    return getRep().getMobilizerTransform(s);
}
const SpatialVec& MobilizedBody::getBodyVelocity(const State& s) const {
    return getRep().getBodyVelocity(s);
}
const SpatialVec& MobilizedBody::getMobilizerVelocity(const State& s) const {
    return getRep().getMobilizerVelocity(s);
}
const SpatialVec& MobilizedBody::getBodyAppliedForces(const State& s) const {
    return getRep().getBodyAppliedForces(s);
}
const SpatialVec& MobilizedBody::getBodyAcceleration(const State& s) const {
    return getRep().getBodyAcceleration(s);
}

SpatialVec& MobilizedBody::updBodyAppliedForces(State& s) const {
    return getRep().updBodyAppliedForces(s);
}

    ////////////////////////
    // MOBILIZED BODY REP //
    ////////////////////////

int MobilizedBody::MobilizedBodyRep::getQIndex(const State& s) const {
    int qStart, nq;
    getMyMatterSubsystemRep()
        .findMobilizerQs(s, MobilizedBodyId(myMobilizedBodyId), qStart, nq);
    return qStart;
}
int MobilizedBody::MobilizedBodyRep::getUIndex(const State& s) const {
    int uStart, nu;
    getMyMatterSubsystemRep()
        .findMobilizerUs(s, MobilizedBodyId(myMobilizedBodyId), uStart, nu);
    return uStart;
}

    /////////////////////////
    // MOBILIZED BODY::PIN //
    /////////////////////////

MobilizedBody::Pin::Pin() {
    rep = new PinRep(); rep->setMyHandle(*this);
}

MobilizedBody::Pin::Pin(MobilizedBody& parent, const Body& body)
{
    rep = new PinRep(); rep->setMyHandle(*this);

    // inb & outb frames are just the parent body's frame and new body's frame
    setBody(body);

    parent.updMatterSubsystem().adoptMobilizedBody(parent.getMobilizedBodyId(),
                                                   *this);
}

MobilizedBody::Pin::Pin(MobilizedBody& parent, const Transform& inbFrame,
                        const Body& body, const Transform& outbFrame)
{
    rep = new PinRep(); rep->setMyHandle(*this);

    setDefaultInboardFrame(inbFrame);
    setDefaultOutboardFrame(outbFrame);
    setBody(body);

    parent.updMatterSubsystem().adoptMobilizedBody(parent.getMobilizedBodyId(),
                                                   *this);
}


Real MobilizedBody::Pin::getDefaultQ() const {
    return getRep().defaultQ;
}

Real& MobilizedBody::Pin::updDefaultQ() {
    return updRep().defaultQ;
}

Real MobilizedBody::Pin::getQ(const State& s) const {
    const MobilizedBodyRep& mbr = MobilizedBody::getRep();
    const int qIndex = mbr.getQIndex(s);
    return mbr.getMyMatterSubsystemRep().getQ(s)[qIndex];
}
Real& MobilizedBody::Pin::updQ(State& s) const {
    const MobilizedBodyRep& mbr = MobilizedBody::getRep();
    const int qIndex = mbr.getQIndex(s);
    return mbr.getMyMatterSubsystemRep().updQ(s)[qIndex];
}

Real MobilizedBody::Pin::getU(const State& s) const {
    const MobilizedBodyRep& mbr = MobilizedBody::getRep();
    const int uIndex = mbr.getUIndex(s);
    return mbr.getMyMatterSubsystemRep().getU(s)[uIndex];
}
Real& MobilizedBody::Pin::updU(State& s) const {
    const MobilizedBodyRep& mbr = MobilizedBody::getRep();
    const int uIndex = mbr.getUIndex(s);
    return mbr.getMyMatterSubsystemRep().updU(s)[uIndex];
}


Real MobilizedBody::Pin::getMobilizerForces(const State& s) const {
    const MobilizedBodyRep& mbr = MobilizedBody::getRep();
    const int uIndex = mbr.getUIndex(s);
    return mbr.getMyMatterSubsystemRep().getAllMobilizerAppliedForces(s)[uIndex];
}
Real& MobilizedBody::Pin::updMobilizerForces(State& s) const {
    const MobilizedBodyRep& mbr = MobilizedBody::getRep();
    const int uIndex = mbr.getUIndex(s);
    return mbr.getMyMatterSubsystemRep().updAllMobilizerAppliedForces(s)[uIndex];
}

    // Pin bookkeeping

bool MobilizedBody::Pin::isInstanceOf(const MobilizedBody& s) {
    return PinRep::isA(s.getRep());
}
const MobilizedBody::Pin& MobilizedBody::Pin::downcast(const MobilizedBody& s) {
    assert(isInstanceOf(s));
    return reinterpret_cast<const Pin&>(s);
}
MobilizedBody::Pin& MobilizedBody::Pin::updDowncast(MobilizedBody& s) {
    assert(isInstanceOf(s));
    return reinterpret_cast<Pin&>(s);
}
const MobilizedBody::Pin::PinRep& MobilizedBody::Pin::getRep() const {
    return dynamic_cast<const PinRep&>(*rep);
}
MobilizedBody::Pin::PinRep& MobilizedBody::Pin::updRep() {
    return dynamic_cast<PinRep&>(*rep);
}

    ////////////////////////////
    // MOBILIZED BODY::SLIDER //
    ////////////////////////////

MobilizedBody::Slider::Slider() {
    rep = new SliderRep(); rep->setMyHandle(*this);
}
bool MobilizedBody::Slider::isInstanceOf(const MobilizedBody& s) {
    return SliderRep::isA(s.getRep());
}
const MobilizedBody::Slider& MobilizedBody::Slider::downcast(const MobilizedBody& s) {
    assert(isInstanceOf(s));
    return reinterpret_cast<const Slider&>(s);
}
MobilizedBody::Slider& MobilizedBody::Slider::updDowncast(MobilizedBody& s) {
    assert(isInstanceOf(s));
    return reinterpret_cast<Slider&>(s);
}
const MobilizedBody::Slider::SliderRep& MobilizedBody::Slider::getRep() const {
    return dynamic_cast<const SliderRep&>(*rep);
}
MobilizedBody::Slider::SliderRep& MobilizedBody::Slider::updRep() {
    return dynamic_cast<SliderRep&>(*rep);
}

    ///////////////////////////////
    // MOBILIZED BODY::UNIVERSAL //
    ///////////////////////////////

MobilizedBody::Universal::Universal() {
    rep = new UniversalRep(); rep->setMyHandle(*this);
}
bool MobilizedBody::Universal::isInstanceOf(const MobilizedBody& s) {
    return UniversalRep::isA(s.getRep());
}
const MobilizedBody::Universal& MobilizedBody::Universal::downcast(const MobilizedBody& s) {
    assert(isInstanceOf(s));
    return reinterpret_cast<const Universal&>(s);
}
MobilizedBody::Universal& MobilizedBody::Universal::updDowncast(MobilizedBody& s) {
    assert(isInstanceOf(s));
    return reinterpret_cast<Universal&>(s);
}
const MobilizedBody::Universal::UniversalRep& MobilizedBody::Universal::getRep() const {
    return dynamic_cast<const UniversalRep&>(*rep);
}
MobilizedBody::Universal::UniversalRep& MobilizedBody::Universal::updRep() {
    return dynamic_cast<UniversalRep&>(*rep);
}

    //////////////////////////////
    // MOBILIZED BODY::CYLINDER //
    //////////////////////////////

MobilizedBody::Cylinder::Cylinder() {
    rep = new CylinderRep(); rep->setMyHandle(*this);
}

MobilizedBody::Cylinder::Cylinder(MobilizedBody& parent, const Body& body)
{
    rep = new CylinderRep(); rep->setMyHandle(*this);

    // inb & outb frames are just the parent body's frame and new body's frame
    setBody(body);

    parent.updMatterSubsystem().adoptMobilizedBody(parent.getMobilizedBodyId(),
                                                   *this);
}

MobilizedBody::Cylinder::Cylinder(MobilizedBody& parent, const Transform& inbFrame,
                                  const Body& body, const Transform& outbFrame)
{
    rep = new CylinderRep(); rep->setMyHandle(*this);

    setDefaultInboardFrame(inbFrame);
    setDefaultOutboardFrame(outbFrame);
    setBody(body);

    parent.updMatterSubsystem().adoptMobilizedBody(parent.getMobilizedBodyId(),
                                                   *this);
}

    // Cylinder bookkeeping

bool MobilizedBody::Cylinder::isInstanceOf(const MobilizedBody& s) {
    return CylinderRep::isA(s.getRep());
}
const MobilizedBody::Cylinder& MobilizedBody::Cylinder::downcast(const MobilizedBody& s) {
    assert(isInstanceOf(s));
    return reinterpret_cast<const Cylinder&>(s);
}
MobilizedBody::Cylinder& MobilizedBody::Cylinder::updDowncast(MobilizedBody& s) {
    assert(isInstanceOf(s));
    return reinterpret_cast<Cylinder&>(s);
}
const MobilizedBody::Cylinder::CylinderRep& MobilizedBody::Cylinder::getRep() const {
    return dynamic_cast<const CylinderRep&>(*rep);
}
MobilizedBody::Cylinder::CylinderRep& MobilizedBody::Cylinder::updRep() {
    return dynamic_cast<CylinderRep&>(*rep);
}

    //////////////////////////////////
    // MOBILIZED BODY::BEND STRETCH //
    //////////////////////////////////

MobilizedBody::BendStretch::BendStretch() {
    rep = new BendStretchRep(); rep->setMyHandle(*this);
}


MobilizedBody::BendStretch::BendStretch(MobilizedBody& parent, const Body& body)
{
    rep = new BendStretchRep(); rep->setMyHandle(*this);

    // inb & outb frames are just the parent body's frame and new body's frame
    setBody(body);

    parent.updMatterSubsystem().adoptMobilizedBody(parent.getMobilizedBodyId(),
                                                   *this);
}

MobilizedBody::BendStretch::BendStretch(MobilizedBody& parent, const Transform& inbFrame,
                                        const Body& body, const Transform& outbFrame)
{
    rep = new BendStretchRep(); rep->setMyHandle(*this);

    setDefaultInboardFrame(inbFrame);
    setDefaultOutboardFrame(outbFrame);
    setBody(body);

    parent.updMatterSubsystem().adoptMobilizedBody(parent.getMobilizedBodyId(),
                                                   *this);
}

    // BendStretch bookkeeping

bool MobilizedBody::BendStretch::isInstanceOf(const MobilizedBody& s) {
    return BendStretchRep::isA(s.getRep());
}
const MobilizedBody::BendStretch& MobilizedBody::BendStretch::downcast(const MobilizedBody& s) {
    assert(isInstanceOf(s));
    return reinterpret_cast<const BendStretch&>(s);
}
MobilizedBody::BendStretch& MobilizedBody::BendStretch::updDowncast(MobilizedBody& s) {
    assert(isInstanceOf(s));
    return reinterpret_cast<BendStretch&>(s);
}
const MobilizedBody::BendStretch::BendStretchRep& MobilizedBody::BendStretch::getRep() const {
    return dynamic_cast<const BendStretchRep&>(*rep);
}
MobilizedBody::BendStretch::BendStretchRep& MobilizedBody::BendStretch::updRep() {
    return dynamic_cast<BendStretchRep&>(*rep);
}

    ////////////////////////////
    // MOBILIZED BODY::PLANAR //
    ////////////////////////////

MobilizedBody::Planar::Planar() {
    rep = new PlanarRep(); rep->setMyHandle(*this);
}

MobilizedBody::Planar::Planar(MobilizedBody& parent, const Body& body)
{
    rep = new PlanarRep(); rep->setMyHandle(*this);

    // inb & outb frames are just the parent body's frame and new body's frame
    setBody(body);

    parent.updMatterSubsystem().adoptMobilizedBody(parent.getMobilizedBodyId(),
                                                   *this);
}

MobilizedBody::Planar::Planar(MobilizedBody& parent, const Transform& inbFrame,
                                  const Body& body, const Transform& outbFrame)
{
    rep = new PlanarRep(); rep->setMyHandle(*this);

    setDefaultInboardFrame(inbFrame);
    setDefaultOutboardFrame(outbFrame);
    setBody(body);

    parent.updMatterSubsystem().adoptMobilizedBody(parent.getMobilizedBodyId(),
                                                   *this);
}

const Vec3& MobilizedBody::Planar::getDefaultQ() const {
    return getRep().defaultQ;
}

Vec3& MobilizedBody::Planar::updDefaultQ() {
    return updRep().defaultQ;
}

const Vec3& MobilizedBody::Planar::getQ(const State& s) const {
    const MobilizedBodyRep& mbr = MobilizedBody::getRep();
    const int qIndex = mbr.getQIndex(s);
    return Vec3::getAs(&mbr.getMyMatterSubsystemRep().getQ(s)[qIndex]);
}
Vec3& MobilizedBody::Planar::updQ(State& s) const {
    const MobilizedBodyRep& mbr = MobilizedBody::getRep();
    const int qIndex = mbr.getQIndex(s);
    return Vec3::updAs(&mbr.getMyMatterSubsystemRep().updQ(s)[qIndex]);
}

const Vec3& MobilizedBody::Planar::getU(const State& s) const {
    const MobilizedBodyRep& mbr = MobilizedBody::getRep();
    const int uIndex = mbr.getUIndex(s);
    return Vec3::getAs(&mbr.getMyMatterSubsystemRep().getU(s)[uIndex]);
}
Vec3& MobilizedBody::Planar::updU(State& s) const {
    const MobilizedBodyRep& mbr = MobilizedBody::getRep();
    const int uIndex = mbr.getUIndex(s);
    return Vec3::updAs(&mbr.getMyMatterSubsystemRep().updU(s)[uIndex]);
}

const Vec3& MobilizedBody::Planar::getMobilizerForces(const State& s) const {
    const MobilizedBodyRep& mbr = MobilizedBody::getRep();
    const int uIndex = mbr.getUIndex(s);
    return Vec3::getAs(&mbr.getMyMatterSubsystemRep().getAllMobilizerAppliedForces(s)[uIndex]);
}
Vec3& MobilizedBody::Planar::updMobilizerForces(State& s) const {
    const MobilizedBodyRep& mbr = MobilizedBody::getRep();
    const int uIndex = mbr.getUIndex(s);
    return Vec3::updAs(&mbr.getMyMatterSubsystemRep().updAllMobilizerAppliedForces(s)[uIndex]);
}

    // Planar mobilized body bookkeeping

bool MobilizedBody::Planar::isInstanceOf(const MobilizedBody& s) {
    return PlanarRep::isA(s.getRep());
}
const MobilizedBody::Planar& MobilizedBody::Planar::downcast(const MobilizedBody& s) {
    assert(isInstanceOf(s));
    return reinterpret_cast<const Planar&>(s);
}
MobilizedBody::Planar& MobilizedBody::Planar::updDowncast(MobilizedBody& s) {
    assert(isInstanceOf(s));
    return reinterpret_cast<Planar&>(s);
}
const MobilizedBody::Planar::PlanarRep& MobilizedBody::Planar::getRep() const {
    return dynamic_cast<const PlanarRep&>(*rep);
}
MobilizedBody::Planar::PlanarRep& MobilizedBody::Planar::updRep() {
    return dynamic_cast<PlanarRep&>(*rep);
}

    ////////////////////////////
    // MOBILIZED BODY::GIMBAL //
    ////////////////////////////

MobilizedBody::Gimbal::Gimbal() {
    rep = new GimbalRep(); rep->setMyHandle(*this);
}
bool MobilizedBody::Gimbal::isInstanceOf(const MobilizedBody& s) {
    return GimbalRep::isA(s.getRep());
}
const MobilizedBody::Gimbal& MobilizedBody::Gimbal::downcast(const MobilizedBody& s) {
    assert(isInstanceOf(s));
    return reinterpret_cast<const Gimbal&>(s);
}
MobilizedBody::Gimbal& MobilizedBody::Gimbal::updDowncast(MobilizedBody& s) {
    assert(isInstanceOf(s));
    return reinterpret_cast<Gimbal&>(s);
}
const MobilizedBody::Gimbal::GimbalRep& MobilizedBody::Gimbal::getRep() const {
    return dynamic_cast<const GimbalRep&>(*rep);
}
MobilizedBody::Gimbal::GimbalRep& MobilizedBody::Gimbal::updRep() {
    return dynamic_cast<GimbalRep&>(*rep);
}

    ///////////////////////////////////////////////////
    // MOBILIZED BODY::BALL (ORIENTATION, SPHERICAL) //
    ///////////////////////////////////////////////////

MobilizedBody::Ball::Ball() {
    rep = new BallRep(); rep->setMyHandle(*this);
}

MobilizedBody::Ball::Ball(MobilizedBody& parent, const Body& body)
{
    rep = new BallRep(); rep->setMyHandle(*this);

    // inb & outb frames are just the parent body's frame and new body's frame
    setBody(body);

    parent.updMatterSubsystem().adoptMobilizedBody(parent.getMobilizedBodyId(),
                                                   *this);
}

MobilizedBody::Ball::Ball(MobilizedBody& parent, const Transform& inbFrame,
                          const Body& body, const Transform& outbFrame)
{
    rep = new BallRep(); rep->setMyHandle(*this);

    setDefaultInboardFrame(inbFrame);
    setDefaultOutboardFrame(outbFrame);
    setBody(body);

    parent.updMatterSubsystem().adoptMobilizedBody(parent.getMobilizedBodyId(),
                                                   *this);
}

    // Ball bookkeeping

bool MobilizedBody::Ball::isInstanceOf(const MobilizedBody& s) {
    return BallRep::isA(s.getRep());
}
const MobilizedBody::Ball& MobilizedBody::Ball::downcast(const MobilizedBody& s) {
    assert(isInstanceOf(s));
    return reinterpret_cast<const Ball&>(s);
}
MobilizedBody::Ball& MobilizedBody::Ball::updDowncast(MobilizedBody& s) {
    assert(isInstanceOf(s));
    return reinterpret_cast<Ball&>(s);
}
const MobilizedBody::Ball::BallRep& MobilizedBody::Ball::getRep() const {
    return dynamic_cast<const BallRep&>(*rep);
}
MobilizedBody::Ball::BallRep& MobilizedBody::Ball::updRep() {
    return dynamic_cast<BallRep&>(*rep);
}

    /////////////////////////////////
    // MOBILIZED BODY::TRANSLATION //
    /////////////////////////////////

MobilizedBody::Translation::Translation() {
    rep = new TranslationRep(); rep->setMyHandle(*this);
}


MobilizedBody::Translation::Translation(MobilizedBody& parent, const Body& body)
{
    rep = new TranslationRep(); rep->setMyHandle(*this);

    // inb & outb frames are just the parent body's frame and new body's frame
    setBody(body);

    parent.updMatterSubsystem().adoptMobilizedBody(parent.getMobilizedBodyId(),
                                                   *this);
}

MobilizedBody::Translation::Translation(MobilizedBody& parent, const Transform& inbFrame,
                          const Body& body, const Transform& outbFrame)
{
    rep = new TranslationRep(); rep->setMyHandle(*this);

    setDefaultInboardFrame(inbFrame);
    setDefaultOutboardFrame(outbFrame);
    setBody(body);

    parent.updMatterSubsystem().adoptMobilizedBody(parent.getMobilizedBodyId(),
                                                   *this);
}

    // Translation mobilizer bookkeeping

bool MobilizedBody::Translation::isInstanceOf(const MobilizedBody& s) {
    return TranslationRep::isA(s.getRep());
}
const MobilizedBody::Translation& MobilizedBody::Translation::downcast(const MobilizedBody& s) {
    assert(isInstanceOf(s));
    return reinterpret_cast<const Translation&>(s);
}
MobilizedBody::Translation& MobilizedBody::Translation::updDowncast(MobilizedBody& s) {
    assert(isInstanceOf(s));
    return reinterpret_cast<Translation&>(s);
}
const MobilizedBody::Translation::TranslationRep& MobilizedBody::Translation::getRep() const {
    return dynamic_cast<const TranslationRep&>(*rep);
}
MobilizedBody::Translation::TranslationRep& MobilizedBody::Translation::updRep() {
    return dynamic_cast<TranslationRep&>(*rep);
}

    //////////////////////////
    // MOBILIZED BODY::FREE //
    //////////////////////////

MobilizedBody::Free::Free() {
    rep = new FreeRep(); rep->setMyHandle(*this);
}

MobilizedBody::Free::Free(MobilizedBody& parent, const Body& body)
{
    rep = new FreeRep(); rep->setMyHandle(*this);

    // inb & outb frames are just the parent body's frame and new body's frame
    setBody(body);

    parent.updMatterSubsystem().adoptMobilizedBody(parent.getMobilizedBodyId(),
                                                   *this);
}

MobilizedBody::Free::Free(MobilizedBody& parent, const Transform& inbFrame,
                          const Body& body, const Transform& outbFrame)
{
    rep = new FreeRep(); rep->setMyHandle(*this);

    setDefaultInboardFrame(inbFrame);
    setDefaultOutboardFrame(outbFrame);
    setBody(body);

    parent.updMatterSubsystem().adoptMobilizedBody(parent.getMobilizedBodyId(),
                                                   *this);
}

    // Free mobilizer bookkeeping

bool MobilizedBody::Free::isInstanceOf(const MobilizedBody& s) {
    return FreeRep::isA(s.getRep());
}
const MobilizedBody::Free& MobilizedBody::Free::downcast(const MobilizedBody& s) {
    assert(isInstanceOf(s));
    return reinterpret_cast<const Free&>(s);
}
MobilizedBody::Free& MobilizedBody::Free::updDowncast(MobilizedBody& s) {
    assert(isInstanceOf(s));
    return reinterpret_cast<Free&>(s);
}
const MobilizedBody::Free::FreeRep& MobilizedBody::Free::getRep() const {
    return dynamic_cast<const FreeRep&>(*rep);
}
MobilizedBody::Free::FreeRep& MobilizedBody::Free::updRep() {
    return dynamic_cast<FreeRep&>(*rep);
}

    //////////////////////////////////////
    // MOBILIZED BODY::LINE ORIENTATION //
    //////////////////////////////////////

MobilizedBody::LineOrientation::LineOrientation() {
    rep = new LineOrientationRep(); rep->setMyHandle(*this);
}
bool MobilizedBody::LineOrientation::isInstanceOf(const MobilizedBody& s) {
    return LineOrientationRep::isA(s.getRep());
}
const MobilizedBody::LineOrientation& MobilizedBody::LineOrientation::downcast(const MobilizedBody& s) {
    assert(isInstanceOf(s));
    return reinterpret_cast<const LineOrientation&>(s);
}
MobilizedBody::LineOrientation& MobilizedBody::LineOrientation::updDowncast(MobilizedBody& s) {
    assert(isInstanceOf(s));
    return reinterpret_cast<LineOrientation&>(s);
}
const MobilizedBody::LineOrientation::LineOrientationRep& MobilizedBody::LineOrientation::getRep() const {
    return dynamic_cast<const LineOrientationRep&>(*rep);
}
MobilizedBody::LineOrientation::LineOrientationRep& MobilizedBody::LineOrientation::updRep() {
    return dynamic_cast<LineOrientationRep&>(*rep);
}

    ///////////////////////////////
    // MOBILIZED BODY::FREE LINE //
    ///////////////////////////////

MobilizedBody::FreeLine::FreeLine() {
    rep = new FreeLineRep(); rep->setMyHandle(*this);
}


MobilizedBody::FreeLine::FreeLine(MobilizedBody& parent, const Body& body)
{
    rep = new FreeLineRep(); rep->setMyHandle(*this);

    // inb & outb frames are just the parent body's frame and new body's frame
    setBody(body);

    parent.updMatterSubsystem().adoptMobilizedBody(parent.getMobilizedBodyId(),
                                                   *this);
}

MobilizedBody::FreeLine::FreeLine(MobilizedBody& parent, const Transform& inbFrame,
                                  const Body& body, const Transform& outbFrame)
{
    rep = new FreeLineRep(); rep->setMyHandle(*this);

    setDefaultInboardFrame(inbFrame);
    setDefaultOutboardFrame(outbFrame);
    setBody(body);

    parent.updMatterSubsystem().adoptMobilizedBody(parent.getMobilizedBodyId(),
                                                   *this);
}

    // FreeLine bookkeeping

bool MobilizedBody::FreeLine::isInstanceOf(const MobilizedBody& s) {
    return FreeLineRep::isA(s.getRep());
}
const MobilizedBody::FreeLine& MobilizedBody::FreeLine::downcast(const MobilizedBody& s) {
    assert(isInstanceOf(s));
    return reinterpret_cast<const FreeLine&>(s);
}
MobilizedBody::FreeLine& MobilizedBody::FreeLine::updDowncast(MobilizedBody& s) {
    assert(isInstanceOf(s));
    return reinterpret_cast<FreeLine&>(s);
}
const MobilizedBody::FreeLine::FreeLineRep& MobilizedBody::FreeLine::getRep() const {
    return dynamic_cast<const FreeLineRep&>(*rep);
}
MobilizedBody::FreeLine::FreeLineRep& MobilizedBody::FreeLine::updRep() {
    return dynamic_cast<FreeLineRep&>(*rep);
}

    //////////////////////////
    // MOBILIZED BODY::WELD //
    //////////////////////////

MobilizedBody::Weld::Weld() {
    rep = new WeldRep(); rep->setMyHandle(*this);
}
bool MobilizedBody::Weld::isInstanceOf(const MobilizedBody& s) {
    return WeldRep::isA(s.getRep());
}
const MobilizedBody::Weld& MobilizedBody::Weld::downcast(const MobilizedBody& s) {
    assert(isInstanceOf(s));
    return reinterpret_cast<const Weld&>(s);
}
MobilizedBody::Weld& MobilizedBody::Weld::updDowncast(MobilizedBody& s) {
    assert(isInstanceOf(s));
    return reinterpret_cast<Weld&>(s);
}
const MobilizedBody::Weld::WeldRep& MobilizedBody::Weld::getRep() const {
    return dynamic_cast<const WeldRep&>(*rep);
}
MobilizedBody::Weld::WeldRep& MobilizedBody::Weld::updRep() {
    return dynamic_cast<WeldRep&>(*rep);
}

    ////////////////////////////////
    // (IM)MOBILIZED BODY::GROUND //
    ////////////////////////////////

MobilizedBody::Ground::Ground() {
    rep = new GroundRep(); rep->setMyHandle(*this);
}
bool MobilizedBody::Ground::isInstanceOf(const MobilizedBody& s) {
    return GroundRep::isA(s.getRep());
}
const MobilizedBody::Ground& MobilizedBody::Ground::downcast(const MobilizedBody& s) {
    assert(isInstanceOf(s));
    return reinterpret_cast<const MobilizedBody::Ground&>(s);
}
MobilizedBody::Ground& MobilizedBody::Ground::updDowncast(MobilizedBody& s) {
    assert(isInstanceOf(s));
    return reinterpret_cast<MobilizedBody::Ground&>(s);
}
const MobilizedBody::Ground::GroundRep& MobilizedBody::Ground::getRep() const {
    return dynamic_cast<const GroundRep&>(*rep);
}
MobilizedBody::Ground::GroundRep& MobilizedBody::Ground::updRep() {
    return dynamic_cast<GroundRep&>(*rep);
}

    ///////////////////////////
    // MOBILIZED BODY::SCREW //
    ///////////////////////////

MobilizedBody::Screw::Screw(Real pitch) {
    rep = new ScrewRep(pitch); rep->setMyHandle(*this);
}

MobilizedBody::Screw::Screw(MobilizedBody& parent, const Body& body, Real pitch)
{
    rep = new ScrewRep(pitch); rep->setMyHandle(*this);

    // inb & outb frames are just the parent body's frame and new body's frame
    setBody(body);

    parent.updMatterSubsystem().adoptMobilizedBody(parent.getMobilizedBodyId(),
                                                   *this);
}

MobilizedBody::Screw::Screw(MobilizedBody& parent, const Transform& inbFrame,
                            const Body& body, const Transform& outbFrame,
                            Real pitch)
{
    rep = new ScrewRep(pitch); rep->setMyHandle(*this);

    setDefaultInboardFrame(inbFrame);
    setDefaultOutboardFrame(outbFrame);
    setBody(body);

    parent.updMatterSubsystem().adoptMobilizedBody(parent.getMobilizedBodyId(),
                                                   *this);
}

MobilizedBody::Screw& MobilizedBody::Screw::setDefaultPitch(Real pitch) {
    updRep().setDefaultPitch(pitch);
    return *this;
}

bool MobilizedBody::Screw::isInstanceOf(const MobilizedBody& s) {
    return ScrewRep::isA(s.getRep());
}
const MobilizedBody::Screw& MobilizedBody::Screw::downcast(const MobilizedBody& s) {
    assert(isInstanceOf(s));
    return reinterpret_cast<const Screw&>(s);
}
MobilizedBody::Screw& MobilizedBody::Screw::updDowncast(MobilizedBody& s) {
    assert(isInstanceOf(s));
    return reinterpret_cast<Screw&>(s);
}
const MobilizedBody::Screw::ScrewRep& MobilizedBody::Screw::getRep() const {
    return dynamic_cast<const ScrewRep&>(*rep);
}
MobilizedBody::Screw::ScrewRep& MobilizedBody::Screw::updRep() {
    return dynamic_cast<ScrewRep&>(*rep);
}

    ////////////////////////////
    // MOBILIZED BODY::CUSTOM //
    ////////////////////////////

MobilizedBody::Custom::Custom(int nMobilities, int nCoordinates) {
    rep = new CustomRep(nMobilities, nCoordinates); rep->setMyHandle(*this);
}
bool MobilizedBody::Custom::isInstanceOf(const MobilizedBody& s) {
    return CustomRep::isA(s.getRep());
}
const MobilizedBody::Custom& MobilizedBody::Custom::downcast(const MobilizedBody& s) {
    assert(isInstanceOf(s));
    return reinterpret_cast<const Custom&>(s);
}
MobilizedBody::Custom& MobilizedBody::Custom::updDowncast(MobilizedBody& s) {
    assert(isInstanceOf(s));
    return reinterpret_cast<Custom&>(s);
}
const MobilizedBody::Custom::CustomRep& MobilizedBody::Custom::getRep() const {
    return dynamic_cast<const CustomRep&>(*rep);
}
MobilizedBody::Custom::CustomRep& MobilizedBody::Custom::updRep() {
    return dynamic_cast<CustomRep&>(*rep);
}

    ////////////////
    // CONSTRAINT //
    ////////////////

bool Constraint::isEmptyHandle() const {return rep==0;}
bool Constraint::isOwnerHandle() const {return rep==0 || rep->myHandle==this;}

void Constraint::disown(Constraint& newOwnerHandle) {
    SimTK_ASSERT_ALWAYS(rep && rep->myHandle==this,
        "disown() not allowed for an empty or non-owner Constraint handle.");
    SimTK_ASSERT_ALWAYS(!newOwnerHandle.rep,
        "disown() can only transfer ownership to an empty Constraint handle.");

    newOwnerHandle.setRep(*rep);
    rep->setMyHandle(newOwnerHandle);
}

Constraint::~Constraint() {
    if (isOwnerHandle()) delete rep; 
    rep=0;
}

// Make this Constraint a non-owner handle referring to the same
// object as the source.
Constraint::Constraint(Constraint& src) : rep(src.rep) {
}

// Make this empty or non-owner handle refer to the same object
// as the source. This is illegal if the current handle is an
// owner.
Constraint& Constraint::operator=(Constraint& src) {
    if (&src != this) {
        SimTK_ASSERT_ALWAYS(!(rep && rep->myHandle==this),
            "You can't reassign the owner handle of a Constraint.");
        rep = src.rep;
    }
    return *this;
}

const SimbodyMatterSubsystem& Constraint::getMatterSubsystem() const {
    SimTK_ASSERT_ALWAYS(isInSubsystem(),
        "getMatterSubsystem() called on a Constraint that is not part of a subsystem.");
    return getRep().getMyMatterSubsystemRep().getMySimbodyMatterSubsystemHandle();
}

ConstraintId Constraint::getConstraintId() const {
    SimTK_ASSERT_ALWAYS(isInSubsystem(),
        "getConstraintId() called on a Constraint that is not part of a subsystem.");
    return rep->myConstraintId;
}

SimbodyMatterSubsystem& Constraint::updMatterSubsystem() {
    SimTK_ASSERT_ALWAYS(isInSubsystem(),
        "updMatterSubsystem() called on a Constraint that is not part of a subsystem.");
    return updRep().updMyMatterSubsystemRep().updMySimbodyMatterSubsystemHandle();
}

bool Constraint::isInSubsystem() const {
    return rep && rep->isInSubsystem();
}

bool Constraint::isInSameSubsystem(const MobilizedBody& body) const {
    return isInSubsystem() && body.isInSubsystem()
           && getMatterSubsystem().isSameSubsystem(body.getMatterSubsystem());
}


    /////////////////////
    // CONSTRAINT::ROD //
    /////////////////////

Constraint::Rod::Rod(MobilizedBody& body1, MobilizedBody& body2, Real defaultRodLength)
{
    SimTK_ASSERT_ALWAYS(body1.isInSubsystem() && body2.isInSubsystem(),
        "Constraint::Rod(): both bodies must already be in a MatterSubsystem.");
    SimTK_ASSERT_ALWAYS(body1.isInSameSubsystem(body2),
        "Constraint::Rod(): both bodies to be connected must be in the same MatterSubsystem.");
    SimTK_ASSERT_ALWAYS(defaultRodLength > 0,
        "Constraint::Rod(): Rod length must always be greater than zero");

    rep = new RodRep(); rep->setMyHandle(*this);

    updRep().body1 = body1.getMobilizedBodyId();
    updRep().body2 = body2.getMobilizedBodyId();
    updRep().defaultRodLength = defaultRodLength;

    body1.updMatterSubsystem().adoptConstraint(*this);
}

Constraint::Rod::Rod(MobilizedBody& body1, const Vec3& point1,
                     MobilizedBody& body2, const Vec3& point2, Real defaultRodLength)
{
    SimTK_ASSERT_ALWAYS(body1.isInSubsystem() && body2.isInSubsystem(),
        "Constraint::Rod(): both bodies must already be in a MatterSubsystem.");
    SimTK_ASSERT_ALWAYS(body1.isInSameSubsystem(body2),
        "Constraint::Rod(): both bodies to be connected must be in the same MatterSubsystem.");
    SimTK_ASSERT_ALWAYS(defaultRodLength > 0,
        "Constraint::Rod(): Rod length must always be greater than zero");

    rep = new RodRep(); rep->setMyHandle(*this);

    updRep().body1 = body1.getMobilizedBodyId();
    updRep().body2 = body2.getMobilizedBodyId();
    updRep().defaultPoint1 = point1;
    updRep().defaultPoint2 = point2;
    updRep().defaultRodLength = defaultRodLength;

    body1.updMatterSubsystem().adoptConstraint(*this);
}

Constraint::Rod& Constraint::Rod::setDefaultPointOnBody1(const Vec3& p1) {
    updRep().defaultPoint1 = p1;
    return *this;
}

Constraint::Rod& Constraint::Rod::setDefaultPointOnBody2(const Vec3& p2) {
    updRep().defaultPoint2 = p2;
    return *this;
}

Constraint::Rod& Constraint::Rod::setDefaultRodLength(Real length) {
    updRep().defaultRodLength = length;
    return *this;
}


MobilizedBodyId Constraint::Rod::getBody1Id() const {
    return getRep().body1;
}
MobilizedBodyId Constraint::Rod::getBody2Id() const {
    return getRep().body2;
}
const Vec3& Constraint::Rod::getDefaultPointOnBody1() const {
    return getRep().defaultPoint1;
}
const Vec3& Constraint::Rod::getDefaultPointOnBody2() const {
    return getRep().defaultPoint2;
}
Real Constraint::Rod::getDefaultRodLength() const {
    return getRep().defaultRodLength;
}



    // Rod bookkeeping //

bool Constraint::Rod::isInstanceOf(const Constraint& s) {
    return RodRep::isA(s.getRep());
}
const Constraint::Rod& Constraint::Rod::downcast(const Constraint& s) {
    assert(isInstanceOf(s));
    return reinterpret_cast<const Rod&>(s);
}
Constraint::Rod& Constraint::Rod::updDowncast(Constraint& s) {
    assert(isInstanceOf(s));
    return reinterpret_cast<Rod&>(s);
}
const Constraint::Rod::RodRep& Constraint::Rod::getRep() const {
    return dynamic_cast<const RodRep&>(*rep);
}
Constraint::Rod::RodRep& Constraint::Rod::updRep() {
    return dynamic_cast<RodRep&>(*rep);
}

    //////////////////////
    // CONSTRAINT::BALL //
    //////////////////////

Constraint::Ball::Ball(MobilizedBody& body1, MobilizedBody& body2)
{
    SimTK_ASSERT_ALWAYS(body1.isInSubsystem() && body2.isInSubsystem(),
        "Constraint::Ball(): both bodies must already be in a MatterSubsystem.");
    SimTK_ASSERT_ALWAYS(body1.isInSameSubsystem(body2),
        "Constraint::Ball(): both bodies to be connected must be in the same MatterSubsystem.");

    rep = new BallRep(); rep->setMyHandle(*this);

    updRep().body1 = body1.getMobilizedBodyId();
    updRep().body2 = body2.getMobilizedBodyId();

    body1.updMatterSubsystem().adoptConstraint(*this);
}

Constraint::Ball::Ball(MobilizedBody& body1, const Vec3& point1,
                       MobilizedBody& body2, const Vec3& point2)
{
    SimTK_ASSERT_ALWAYS(body1.isInSubsystem() && body2.isInSubsystem(),
        "Constraint::Ball(): both bodies must already be in a MatterSubsystem.");
    SimTK_ASSERT_ALWAYS(body1.isInSameSubsystem(body2),
        "Constraint::Ball(): both bodies to be connected must be in the same MatterSubsystem.");

    rep = new BallRep(); rep->setMyHandle(*this);

    updRep().body1 = body1.getMobilizedBodyId();
    updRep().body2 = body2.getMobilizedBodyId();
    updRep().defaultPoint1 = point1;
    updRep().defaultPoint2 = point2;

    body1.updMatterSubsystem().adoptConstraint(*this);
}

Constraint::Ball& Constraint::Ball::setDefaultPointOnBody1(const Vec3& p1) {
    updRep().defaultPoint1 = p1;
    return *this;
}

Constraint::Ball& Constraint::Ball::setDefaultPointOnBody2(const Vec3& p2) {
    updRep().defaultPoint2 = p2;
    return *this;
}

MobilizedBodyId Constraint::Ball::getBody1Id() const {
    return getRep().body1;
}
MobilizedBodyId Constraint::Ball::getBody2Id() const {
    return getRep().body2;
}
const Vec3& Constraint::Ball::getDefaultPointOnBody1() const {
    return getRep().defaultPoint1;
}
const Vec3& Constraint::Ball::getDefaultPointOnBody2() const {
    return getRep().defaultPoint2;
}


    // Ball bookkeeping //

bool Constraint::Ball::isInstanceOf(const Constraint& s) {
    return BallRep::isA(s.getRep());
}
const Constraint::Ball& Constraint::Ball::downcast(const Constraint& s) {
    assert(isInstanceOf(s));
    return reinterpret_cast<const Ball&>(s);
}
Constraint::Ball& Constraint::Ball::updDowncast(Constraint& s) {
    assert(isInstanceOf(s));
    return reinterpret_cast<Ball&>(s);
}
const Constraint::Ball::BallRep& Constraint::Ball::getRep() const {
    return dynamic_cast<const BallRep&>(*rep);
}
Constraint::Ball::BallRep& Constraint::Ball::updRep() {
    return dynamic_cast<BallRep&>(*rep);
}

    //////////////////////
    // CONSTRAINT::WELD //
    //////////////////////

Constraint::Weld::Weld(MobilizedBody& body1, MobilizedBody& body2)
{
    SimTK_ASSERT_ALWAYS(body1.isInSubsystem() && body2.isInSubsystem(),
        "Constraint::Weld(): both bodies must already be in a MatterSubsystem.");
    SimTK_ASSERT_ALWAYS(body1.isInSameSubsystem(body2),
        "Constraint::Weld(): both bodies to be connected must be in the same MatterSubsystem.");

    rep = new WeldRep(); rep->setMyHandle(*this);

    updRep().body1 = body1.getMobilizedBodyId();
    updRep().body2 = body2.getMobilizedBodyId();

    body1.updMatterSubsystem().adoptConstraint(*this);
}

Constraint::Weld::Weld(MobilizedBody& body1, const Transform& frame1,
                       MobilizedBody& body2, const Transform& frame2)
{
    SimTK_ASSERT_ALWAYS(body1.isInSubsystem() && body2.isInSubsystem(),
        "Constraint::Weld(): both bodies must already be in a MatterSubsystem.");
    SimTK_ASSERT_ALWAYS(body1.isInSameSubsystem(body2),
        "Constraint::Weld(): both bodies to be connected must be in the same MatterSubsystem.");

    rep = new WeldRep(); rep->setMyHandle(*this);

    updRep().body1 = body1.getMobilizedBodyId();
    updRep().body2 = body2.getMobilizedBodyId();
    updRep().defaultFrame1 = frame1;
    updRep().defaultFrame2 = frame2;

    body1.updMatterSubsystem().adoptConstraint(*this);
}

Constraint::Weld& Constraint::Weld::setDefaultFrameOnBody1(const Transform& f1) {
    updRep().defaultFrame1 = f1;
    return *this;
}

Constraint::Weld& Constraint::Weld::setDefaultFrameOnBody2(const Transform& f2) {
    updRep().defaultFrame2 = f2;
    return *this;
}

MobilizedBodyId Constraint::Weld::getBody1Id() const {
    return getRep().body1;
}
MobilizedBodyId Constraint::Weld::getBody2Id() const {
    return getRep().body2;
}
const Transform& Constraint::Weld::getDefaultFrameOnBody1() const {
    return getRep().defaultFrame1;
}
const Transform& Constraint::Weld::getDefaultFrameOnBody2() const {
    return getRep().defaultFrame2;
}


    // Weld bookkeeping //

bool Constraint::Weld::isInstanceOf(const Constraint& s) {
    return WeldRep::isA(s.getRep());
}
const Constraint::Weld& Constraint::Weld::downcast(const Constraint& s) {
    assert(isInstanceOf(s));
    return reinterpret_cast<const Weld&>(s);
}
Constraint::Weld& Constraint::Weld::updDowncast(Constraint& s) {
    assert(isInstanceOf(s));
    return reinterpret_cast<Weld&>(s);
}
const Constraint::Weld::WeldRep& Constraint::Weld::getRep() const {
    return dynamic_cast<const WeldRep&>(*rep);
}
Constraint::Weld::WeldRep& Constraint::Weld::updRep() {
    return dynamic_cast<WeldRep&>(*rep);
}
} // namespace SimTK
