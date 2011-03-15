/*
 *  Node.cpp
 *  Kepler
 *
 *  Created by Robert Hodgin on 2/25/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#include "cinder/app/AppBasic.h"
#include "cinder/Rand.h"
#include "cinder/gl/gl.h"
#include "cinder/Text.h"
#include "Globals.h"
#include "Node.h"

using namespace ci;
using namespace std;

Node::Node( Node *parent, int index, const Font &font )
	: mParentNode( parent ), mIndex( index ), mFont( font )
{
	if( mParentNode ){
		initWithParent();
	} else {
		init();
	}

	// TODO: restore this behavior
	//if( mName.size() == 0 ){
	//	mName = "untitled";
	//}
	
	//createNameTexture();
	
	mScreenPos			= Vec2f::zero();
	mEclipsePer			= 1.0f;
	mTransPos			= mPos;
    mTransPrevPos       = mPos;
    for( int i=0; i<50; i++ ){
        mTransPosVector.push_back( mTransPos );
    }
	mCamZVel			= 0.0f;
	mStartAngle			= Rand::randFloat( TWO_PI );
	mOrbitAngle			= mStartAngle;
	mOrbitPeriod		= Rand::randFloat( 25.0f, 150.0f );
	mOrbitRadius		= 0.01f;
	mAngularVelocity	= 0.0f;
	mPercentPlayed		= 0.0f;
	mDistFromCamZAxis	= 1000.0f;
	mDistFromCamZAxisPer = 1.0f;
	mPlanetTexIndex		= 0;
		
	mIsSelected			= false;
	mIsHighlighted		= false;
}

void Node::init()
{
	mGen				= G_ARTIST_LEVEL;
	mRadius				= 2.0f;
	mPos				= Rand::randVec3f();
	mPrevPos			= mPos;
	mVel				= Vec3f::zero();
	mOrbitRadiusDest	= 0.0f;
	mOrbitPeriod		= 0.0f;
}

void Node::initWithParent()
{
	mGen				= mParentNode->mGen + 1;
	mRadius				= mParentNode->mRadius * 0.02f;
	mPos				= mParentNode->mPos;
	mPrevPos			= mParentNode->mPos;
	mVel				= mParentNode->mVel;
	mOrbitPeriod		= Rand::randFloat( 25.0f, 150.0f );
}

void Node::createNameTexture()
{
	TextLayout layout;
	layout.setFont( mFont );
	layout.setColor( Color( 1.0f, 1.0f, 1.0f ) );
	layout.addLine( getName() );
	Surface8u nameSurface	= Surface8u( layout.render( true, false ) );
	mNameTex				= gl::Texture( nameSurface );

	if( mGen == G_TRACK_LEVEL ){
		Surface8u planetSurface = Surface( 512, 256, true, SurfaceChannelOrder::RGBA );
		Vec2i offset = Vec2i( 60, 128 - mNameTex.getHeight() * 0.5f );
		planetSurface.copyFrom( nameSurface, nameSurface.getBounds(), offset );
	
		mPlanetTex = gl::Texture( planetSurface );
	}
}

void Node::update( const Matrix44f &mat )
{
	mOrbitRadius -= ( mOrbitRadius - mOrbitRadiusDest ) * 0.1f;
	mMatrix         = mat;
    mTransPrevPos	= mTransPos;
	mTransPos       = mMatrix * mPos;
    
    for( int i=49; i>0; i-- ){
        mTransPosVector[i] = mTransPosVector[i-1];
    }
    mTransPosVector[0] = mTransPos;
    
	mSphere.setCenter( mTransPos );
	
	mZoomPer = constrain( 1.0f - abs( G_ZOOM-mGen+1.0f ), 0.0f, 0.75f );
	mZoomPer = pow( mZoomPer, 3.0f );
		
	if( mIsSelected ){
		if( mGen == G_TRACK_LEVEL ){
			mZoomPer = 1.0f - mZoomPer;
		} else {
			mZoomPer = 1.0f;
		}
	}

	for( vector<Node*>::iterator nodeIt = mChildNodes.begin(); nodeIt != mChildNodes.end(); ++nodeIt ){
		(*nodeIt)->update( mat );
	}
}

void Node::updateGraphics( const CameraPersp &cam, const Vec3f &bbRight, const Vec3f &bbUp )
{
    if( mGen >= G_ALBUM_LEVEL ){
        if( mIsSelected ){
            mSphereRes		-= ( mSphereRes - 32 ) * 0.1f;
            mCamDistAlpha	-= ( mCamDistAlpha - 1.0f ) * 0.05f;
        } else {
            mSphereRes		-= ( mSphereRes - 14 ) * 0.1f;
            mCamDistAlpha	-= ( mCamDistAlpha - 0.0f ) * 0.05f;
        }
    }
    
	mBbRight            = bbRight;
	mBbUp               = bbUp;
	
    
	if( mIsHighlighted ){
        mScreenPos              = cam.worldToScreen( mTransPos, app::getWindowWidth(), app::getWindowHeight() );
		mPrevDistFromCamZAxis	= mDistFromCamZAxis;
		mDistFromCamZAxis		= cam.worldToEyeDepth( mTransPos );
		mCamZVel				= mDistFromCamZAxis - mPrevDistFromCamZAxis;
		mDistFromCamZAxisPer	= constrain( mDistFromCamZAxis * -0.35f, 0.0f, 1.0f );
		mSphereScreenRadius = cam.getScreenRadius( mSphere, app::getWindowWidth(), app::getWindowHeight() ) * 0.4f;
	}
	
	for( vector<Node*>::iterator nodeIt = mChildNodes.begin(); nodeIt != mChildNodes.end(); ++nodeIt ){
		(*nodeIt)->updateGraphics( cam, mBbRight, mBbUp );
	}
}

void Node::drawStar()
{
	for( vector<Node*>::iterator nodeIt = mChildNodes.begin(); nodeIt != mChildNodes.end(); ++nodeIt ){
		(*nodeIt)->drawStar();
	}
}

void Node::drawStarGlow()
{
	for( vector<Node*>::iterator nodeIt = mChildNodes.begin(); nodeIt != mChildNodes.end(); ++nodeIt ){
		(*nodeIt)->drawStarGlow();
	}
}

void Node::drawName( const CameraPersp &cam, float pinchAlphaOffset )
{	
	if( cam.worldToEyeDepth( mTransPos ) < 0 ){
		if( mIsSelected ){
			gl::color( ColorA( 1.0f, 1.0f, 1.0f, mZoomPer * pinchAlphaOffset ) );
		} else {
			gl::color( ColorA( mColor, mZoomPer * pinchAlphaOffset ) );
		}

		if (mNameTex == NULL) {
			createNameTexture();
		}
		
		Vec2f offset = Vec2f( mSphereScreenRadius * 0.4f, -mNameTex.getHeight() * 0.5f );
		gl::draw( mNameTex, mScreenPos + offset );
	}
	
	for( vector<Node*>::iterator nodeIt = mChildNodes.begin(); nodeIt != mChildNodes.end(); ++nodeIt ){
		(*nodeIt)->drawName( cam, pinchAlphaOffset );
	}
}

void Node::drawOrbitRing()
{
	for( vector<Node*>::iterator nodeIt = mChildNodes.begin(); nodeIt != mChildNodes.end(); ++nodeIt ){
		(*nodeIt)->drawOrbitRing();
	}
}

void Node::drawPlanet( const Matrix44f &accelMatrix, const vector<gl::Texture> &planets )
{
	for( vector<Node*>::iterator nodeIt = mChildNodes.begin(); nodeIt != mChildNodes.end(); ++nodeIt ){
		(*nodeIt)->drawPlanet( accelMatrix, planets );
	}
}

void Node::drawClouds( const Matrix44f &accelMatrix, const vector<gl::Texture> &clouds )
{
	for( vector<Node*>::iterator nodeIt = mChildNodes.begin(); nodeIt != mChildNodes.end(); ++nodeIt ){
		(*nodeIt)->drawClouds( accelMatrix, clouds );
	}
}

void Node::drawRings( const gl::Texture &tex )
{
	for( vector<Node*>::iterator nodeIt = mChildNodes.begin(); nodeIt != mChildNodes.end(); ++nodeIt ){
		(*nodeIt)->drawRings( tex );
	}
}

void Node::drawAtmosphere()
{
	for( vector<Node*>::iterator nodeIt = mChildNodes.begin(); nodeIt != mChildNodes.end(); ++nodeIt ){
		(*nodeIt)->drawAtmosphere();
	}
}

void Node::checkForSphereIntersect( vector<Node*> &nodes, const Ray &ray, Matrix44f &mat )
{
	mSphere.setCenter( mat.transformPointAffine( mPos ) );

	if( mSphere.intersects( ray ) && mIsHighlighted && ! mIsSelected ){
		std::cout << "HIT FOUND" << std::endl;
		nodes.push_back( this );
	}
	
	vector<Node*>::iterator nodeIt;
	for( nodeIt = mChildNodes.begin(); nodeIt != mChildNodes.end(); ++nodeIt ){
		(*nodeIt)->checkForSphereIntersect( nodes, ray, mat );
	}
}

void Node::checkForNameTouch( vector<Node*> &nodes, const Vec2f &pos )
{
	if (mNameTex != NULL) {
		
		Rectf r = Rectf( mScreenPos.x - 20, mScreenPos.y - 15, mScreenPos.x + mNameTex.getWidth() + 10, mScreenPos.y + mNameTex.getHeight() + 15 );
		
		if( r.contains( pos ) && mIsHighlighted && ! mIsSelected ){
			std::cout << "HIT FOUND" << std::endl;
			nodes.push_back( this );
		}
		
		vector<Node*>::iterator nodeIt;
		for( nodeIt = mChildNodes.begin(); nodeIt != mChildNodes.end(); ++nodeIt ){
			(*nodeIt)->checkForNameTouch( nodes, pos );
		}
	}
}

void Node::select()
{
	mIsSelected = true;
}

void Node::deselect()
{
	// TODO: Instead of killing them right away, sentence them to die but only after
	// their gen is 1.0 greater than the current zoom level. 
	mIsSelected = false;
	for( vector<Node*>::iterator nodeIt = mChildNodes.begin(); nodeIt != mChildNodes.end(); ++nodeIt ){
		delete (*nodeIt);
	}
	mChildNodes.clear();
}


