/*
 * This sample illustrates how to get audio data from an input device, such as a microphone,
 * with audio::InputDeviceNode. It then visualizes the input in the frequency domain. The frequency
 * spectrum analysis is accomplished with an audio::MonitorSpectralNode.
 *
 * The plot is similar to a typical spectrogram, where the x-axis represents the linear
 * frequency bins (0 - samplerate / 2) and the y-axis is the magnitude of the frequency
 * bin in normalized decibels (0 - 100).
 *
 * author: Richard Eakin (2014)
 */

#include "cinder/app/App.h"
#include "cinder/gl/gl.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/TextureFont.h"

#include "cinder/audio/audio.h"
#include "../../common/AudioDrawUtils.h"

#include "BTrack.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class InputAnalyzerBeatDetection : public App {
  public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;

	void drawSpectralCentroid();
	void drawLabels();
	void printBinInfo( int mouseX );

    BTrack                          btrack;
    
    audio::InputDeviceNodeRef		mInputDeviceNode;
	audio::MonitorSpectralNodeRef	mMonitorSpectralNode;
	vector<float>					mMagSpectrum;

	SpectrumPlot					mSpectrumPlot;
	gl::TextureFontRef				mTextureFont;
};

void InputAnalyzerBeatDetection::setup()
{
	auto ctx = audio::Context::master();

	// The InputDeviceNode is platform-specific, so you create it using a special method on the Context:
	mInputDeviceNode = ctx->createInputDeviceNode();

    BTrack btrack(1024, 2048);
    
	// By providing an FFT size double that of the window size, we 'zero-pad' the analysis data, which gives
	// an increase in resolution of the resulting spectrum data.
	auto monitorFormat = audio::MonitorSpectralNode::Format().fftSize( 2048 ).windowSize( 1024 );
	mMonitorSpectralNode = ctx->makeNode( new audio::MonitorSpectralNode( monitorFormat ) );
    
	mInputDeviceNode >> mMonitorSpectralNode;

	mInputDeviceNode->enable();
	ctx->enable();

	getWindow()->setTitle( mInputDeviceNode->getDevice()->getName() );
}

void InputAnalyzerBeatDetection::mouseDown( MouseEvent event )
{
    audio::Buffer buffer = mMonitorSpectralNode->getBuffer();
    console() << "number of channels: " << buffer.getNumChannels() << std::endl;
    console() << "number of frames: " << buffer.getNumFrames() << std::endl;
    console() << "size: " << buffer.getSize() << std::endl;
    
	if( mSpectrumPlot.getBounds().contains( event.getPos() ) )
		printBinInfo( event.getX() );
}

void InputAnalyzerBeatDetection::update()
{
	mSpectrumPlot.setBounds( Rectf( 40, 40, (float)getWindowWidth() - 40, (float)getWindowHeight() - 40 ) );

    // Grab audio buffer and pass to btrack
    vector<double> samples;
    audio::Buffer buffer = mMonitorSpectralNode->getBuffer();
    for (int i = 0; i < buffer.getSize(); i++) {
        samples.push_back((double) buffer[i]);
    }
    btrack.processAudioFrame(samples.data());
    
	// We copy the magnitude spectrum out from the Node on the main thread, once per update:
	mMagSpectrum = mMonitorSpectralNode->getMagSpectrum();
}

void InputAnalyzerBeatDetection::draw()
{
	gl::clear();
	gl::enableAlphaBlending();

	mSpectrumPlot.draw( mMagSpectrum );
	drawSpectralCentroid();
	drawLabels();
    
    if(btrack.beatDueInCurrentFrame()) {
        gl::pushModelView();
            gl::color(1, 1, 1);
            gl::drawSolidRect({0.0f, 0.0f, (float)getWindowWidth(), (float)getWindowHeight()});
            gl::color(0, 0, 0);
            string beatLabel = "BEAT";
            mTextureFont->drawString( beatLabel, vec2( getWindowCenter().x - mTextureFont->measureString( beatLabel ).x / 2, (float)getWindowHeight()/2 ) );
        gl::popModelView();
    }
    gl::color( 0, 0.9f, 0.9f );
}

void InputAnalyzerBeatDetection::drawSpectralCentroid()
{
	// The spectral centroid is largely correlated with 'brightness' of a sound. It is the center of mass of all frequency values.
	// See the note on audio::MonitorSpectralNode::getSpectralCentroid() - it may be analyzing a more recent magnitude spectrum
	// than what we're drawing in the SpectrumPlot. It is not a problem for this simple sample, but if you need a more precise
	// value, use audio::dsp::spectralCentroid() directly.

	float spectralCentroid = mMonitorSpectralNode->getSpectralCentroid();

	float nyquist = (float)audio::master()->getSampleRate() / 2.0f;
	Rectf bounds = mSpectrumPlot.getBounds();

	float freqNormalized = spectralCentroid / nyquist;
	float barCenter = bounds.x1 + freqNormalized * bounds.getWidth();
	Rectf verticalBar = { barCenter - 2, bounds.y1, barCenter + 2, bounds.y2 };

	gl::ScopedColor colorScope( 0.85f, 0.45f, 0, 0.4f ); // transparent orange
	gl::drawSolidRect( verticalBar );
}

void InputAnalyzerBeatDetection::drawLabels()
{
	if( ! mTextureFont )
		mTextureFont = gl::TextureFont::create( Font( Font::getDefault().getName(), 16 ) );

	// draw x-axis label
	string freqLabel = "Frequency (hertz)";
	mTextureFont->drawString( freqLabel, vec2( getWindowCenter().x - mTextureFont->measureString( freqLabel ).x / 2, (float)getWindowHeight() - 20 ) );

	// draw y-axis label
	string dbLabel = "Magnitude (decibels, linear)";
	gl::pushModelView();
		gl::translate( 30, getWindowCenter().y + mTextureFont->measureString( dbLabel ).x / 2 );
		gl::rotate( -M_PI / 2 );
		mTextureFont->drawString( dbLabel, vec2( 0 ) );
	gl::popModelView();
}

void InputAnalyzerBeatDetection::printBinInfo( int mouseX )
{
	size_t numBins = mMonitorSpectralNode->getFftSize() / 2;
	size_t bin = std::min( numBins - 1, size_t( ( numBins * ( mouseX - mSpectrumPlot.getBounds().x1 ) ) / mSpectrumPlot.getBounds().getWidth() ) );

	float binFreqWidth = mMonitorSpectralNode->getFreqForBin( 1 ) - mMonitorSpectralNode->getFreqForBin( 0 );
	float freq = mMonitorSpectralNode->getFreqForBin( bin );
	float mag = audio::linearToDecibel( mMagSpectrum[bin] );

	console() << "bin: " << bin << ", freqency (hertz): " << freq << " - " << freq + binFreqWidth << ", magnitude (decibels): " << mag << endl;
}

CINDER_APP( InputAnalyzerBeatDetection, RendererGl( RendererGl::Options().msaa( 8 ) ) )
