#ifndef HELLOVRUI_INCLUDED
#define HELLOVRUI_INCLUDED

#include <GL/gl.h>
#include <GL/GLMaterial.h>
#include <GL/GLObject.h>
#include <GLMotif/ToggleButton.h>
#include <Vrui/ToolManager.h>
#include <Vrui/Application.h>

#include "vtkSmartPointer.h"

/* Forward declarations: */
namespace Misc {
class CallbackData;
}
namespace GLMotif {
class Popup;
class PopupMenu;
class PopupWindow;
class TextField;
}

class vtkExternalOpenGLCamera;
class vtkExternalOpenGLRenderWindow;
class vtkExternalOpenGLRenderer;

class HelloVrui:public Vrui::Application,public GLObject
	{
	/* Embedded classes: */
	private:
	struct DataItem:public GLObject::DataItem
		{
		/* Elements: */
		public:
		bool hasVertexBufferObjectExtension; // Flag if buffer objects are supported by the local GL
		GLuint surfaceVertexBufferObjectId; // Vertex buffer object ID for Earth surface
		GLuint surfaceIndexBufferObjectId; // Index buffer object ID for Earth surface
		GLuint surfaceTextureObjectId; // Texture object ID for Earth surface texture
		GLuint displayListIdBase; // Base ID of set of display lists for Earth model components

		/* Constructors and destructors: */
		DataItem(void);
		virtual ~DataItem(void);
		};

	/* Elements: */
	bool showSurface; // Flag if the surface is rendered
	bool surfaceTransparent; // Flag if the surface is rendered transparently
	bool showGrid; // Flag if the grid is rendered
	GLMaterial surfaceMaterial; // OpenGL material properties for the surface
	GLMotif::PopupMenu* mainMenu; // The program's main menu

	/* Private methods: */
	GLMotif::Popup* createRenderTogglesMenu(void); // Creates the "Rendering Modes" submenu
	GLMotif::PopupMenu* createMainMenu(void); // Creates the program's main menu

	/* Constructors and destructors: */
	public:
	HelloVrui(int& argc,char**& argv);
	virtual ~HelloVrui(void);
	void drawCube(void) const;

	/* Methods: */
	virtual void initContext(GLContextData& contextData) const;
	virtual void toolCreationCallback(Vrui::ToolManager::ToolCreationCallbackData* cbData);
	virtual void frame(void);
	virtual void display(GLContextData& contextData) const;
	void menuToggleSelectCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData);
	void centerDisplayCallback(Misc::CallbackData* cbData);

        vtkSmartPointer<vtkExternalOpenGLCamera> cam;
        vtkSmartPointer<vtkExternalOpenGLRenderWindow> renWin;
        vtkSmartPointer<vtkExternalOpenGLRenderer> ren;
        bool drawGLCube;
        void transposeMatrix4x4(double*, double*) const;
	};

#endif
