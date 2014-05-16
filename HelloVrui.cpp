#define CLIP_SCREEN 0

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <vector>
#include <Misc/FunctionCalls.h>
#include <Misc/ThrowStdErr.h>
#include <Math/Math.h>
#include <Math/Constants.h>
#include <Geometry/LinearUnit.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLMatrixTemplates.h>
#include <GL/GLMaterial.h>
#include <GL/GLColorMap.h>
#include <GL/GLContextData.h>
#include <GL/GLModels.h>
#include <GL/GLGeometryWrappers.h>
#include <GL/GLExtensionManager.h>
#include <GL/Extensions/GLARBVertexBufferObject.h>
#include <GL/GLTransformationWrappers.h>
#include <GL/GLFrustum.h>
#include <GLMotif/StyleSheet.h>
#include <GLMotif/WidgetManager.h>
#include <GLMotif/Blind.h>
#include <GLMotif/Label.h>
#include <GLMotif/Button.h>
#include <GLMotif/CascadeButton.h>
#include <GLMotif/Menu.h>
#include <GLMotif/SubMenu.h>
#include <GLMotif/Popup.h>
#include <GLMotif/PopupMenu.h>
#include <GLMotif/PopupWindow.h>
#include <GLMotif/RowColumn.h>
#include <GLMotif/TextField.h>
#include <SceneGraph/NodeCreator.h>
#include <SceneGraph/GroupNode.h>
#include <SceneGraph/VRMLFile.h>
#include <SceneGraph/GLRenderState.h>
#include <Vrui/Vrui.h>
#include <Vrui/CoordinateManager.h>
#include <Vrui/Viewer.h>
#if CLIP_SCREEN
#include <Vrui/VRScreen.h>
#endif
#include <Vrui/Application.h>
#include <Vrui/SceneGraphSupport.h>
#include <Vrui/VRWindow.h>

#include "HelloVrui.h"

// VTK includes
#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkCubeSource.h>
#include <vtkExternalOpenGLCamera.h>
#include <vtkExternalOpenGLRenderer.h>
#include "vtkExternalOpenGLRenderWindow.h"
#include <vtkLightCollection.h>
#include <vtkLight.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkSphereSource.h>
/*****************************************
Methods of class HelloVrui::DataItem:
*****************************************/

HelloVrui::DataItem::DataItem(void)
	:hasVertexBufferObjectExtension(false), // GLARBVertexBufferObject::isSupported()),
	 surfaceVertexBufferObjectId(0),surfaceIndexBufferObjectId(0),
	 surfaceTextureObjectId(0),
	 displayListIdBase(0)
	{
	/* Use buffer objects for the Earth surface if supported: */
	if(hasVertexBufferObjectExtension)
		{
		/* Initialize the vertex buffer object extension: */
		GLARBVertexBufferObject::initExtension();

		/* Create vertex buffer objects: */
		GLuint bufferObjectIds[2];
		glGenBuffersARB(2,bufferObjectIds);
		surfaceVertexBufferObjectId=bufferObjectIds[0];
		surfaceIndexBufferObjectId=bufferObjectIds[1];
		}

	/* Generate a texture object for the Earth's surface texture: */
	glGenTextures(1,&surfaceTextureObjectId);

	/* Generate display lists for the Earth model components: */
	displayListIdBase=glGenLists(4);
	}

HelloVrui::DataItem::~DataItem(void)
	{
	if(hasVertexBufferObjectExtension)
		{
		/* Delete vertex buffer objects: */
		GLuint bufferObjectIds[2];
		bufferObjectIds[0]=surfaceVertexBufferObjectId;
		bufferObjectIds[1]=surfaceIndexBufferObjectId;
		glDeleteBuffersARB(2,bufferObjectIds);
		}

	/* Delete the Earth surface texture object: */
	glDeleteTextures(1,&surfaceTextureObjectId);

	/* Delete the Earth model components display lists: */
	glDeleteLists(displayListIdBase,4);
	}

/*******************************
Methods of class HelloVrui:
*******************************/

GLMotif::Popup* HelloVrui::createRenderTogglesMenu(void)
	{
	/* Create the submenu's top-level shell: */
	GLMotif::Popup* renderTogglesMenuPopup=new GLMotif::Popup("RenderTogglesMenuPopup",Vrui::getWidgetManager());

	/* Create the array of render toggle buttons inside the top-level shell: */
	GLMotif::SubMenu* renderTogglesMenu=new GLMotif::SubMenu("RenderTogglesMenu",renderTogglesMenuPopup,false);

	/* Create a toggle button to render the Earth's surface: */
	GLMotif::ToggleButton* showSurfaceToggle=new GLMotif::ToggleButton("ShowSurfaceToggle",renderTogglesMenu,"Show Surface");
	showSurfaceToggle->setToggle(showSurface);
	showSurfaceToggle->getValueChangedCallbacks().add(this,&HelloVrui::menuToggleSelectCallback);

	/* Create a toggle button to render the Earth's surface transparently: */
	GLMotif::ToggleButton* surfaceTransparentToggle=new GLMotif::ToggleButton("SurfaceTransparentToggle",renderTogglesMenu,"Surface Transparent");
	surfaceTransparentToggle->setToggle(surfaceTransparent);
	surfaceTransparentToggle->getValueChangedCallbacks().add(this,&HelloVrui::menuToggleSelectCallback);

	/* Create a toggle button to render the lat/long grid: */
	GLMotif::ToggleButton* showGridToggle=new GLMotif::ToggleButton("ShowGridToggle",renderTogglesMenu,"Show Grid");
	showGridToggle->setToggle(showGrid);
	showGridToggle->getValueChangedCallbacks().add(this,&HelloVrui::menuToggleSelectCallback);

	/* Calculate the submenu's proper layout: */
	renderTogglesMenu->manageChild();

	/* Return the created top-level shell: */
	return renderTogglesMenuPopup;
	}

GLMotif::PopupMenu* HelloVrui::createMainMenu(void)
	{
	/* Create a top-level shell for the main menu: */
	GLMotif::PopupMenu* mainMenuPopup=new GLMotif::PopupMenu("MainMenuPopup",Vrui::getWidgetManager());
	mainMenuPopup->setTitle("Interactive Globe");

	/* Create the actual menu inside the top-level shell: */
	GLMotif::Menu* mainMenu=new GLMotif::Menu("MainMenu",mainMenuPopup,false);

	/* Create a cascade button to show the "Rendering Modes" submenu: */
	GLMotif::CascadeButton* renderTogglesCascade=new GLMotif::CascadeButton("RenderTogglesCascade",mainMenu,"Rendering Modes");
	renderTogglesCascade->setPopup(createRenderTogglesMenu());

	/* Create a button to reset the navigation coordinates to the default (showing the entire Earth): */
	GLMotif::Button* centerDisplayButton=new GLMotif::Button("CenterDisplayButton",mainMenu,"Center Display");
	centerDisplayButton->getSelectCallbacks().add(this,&HelloVrui::centerDisplayCallback);

	/* Calculate the main menu's proper layout: */
	mainMenu->manageChild();

	/* Return the created top-level shell: */
	return mainMenuPopup;
	}

HelloVrui::HelloVrui(int& argc,char**& argv)
	:Vrui::Application(argc,argv),
	 showSurface(true),surfaceTransparent(false),
	 surfaceMaterial(GLMaterial::Color(1.0f,1.0f,1.0f,0.333f),GLMaterial::Color(0.333f,0.333f,0.333f),10.0f),
	 showGrid(true),
	 mainMenu(0),
         drawGLCube(false)
	{
        if(argc > 1)
          {
          if(std::string(argv[1]) == "--gl")
            {
            this->drawGLCube = true;
            }
          }
	/* Create the user interface: */
	mainMenu=createMainMenu();
	Vrui::setMainMenu(mainMenu);

	/* Initialize Vrui navigation transformation: */
	centerDisplayCallback(0);

        this->renWin =
          vtkSmartPointer<vtkExternalOpenGLRenderWindow>::New();
        this->ren = vtkSmartPointer<vtkExternalOpenGLRenderer>::New();
        this->cam = vtkSmartPointer<vtkExternalOpenGLCamera>::New();
        this->ren->SetActiveCamera(this->cam);
        this->ren->SetClearBuffers(1);
	}

HelloVrui::~HelloVrui(void)
	{
	/* Delete the user interface: */
	delete mainMenu;
	}

void HelloVrui::initContext(GLContextData& contextData) const
	{
	/* Create a new context data item: */
	DataItem* dataItem=new DataItem();
	contextData.addDataItem(this,dataItem);

        this->renWin->AddRenderer(this->ren.GetPointer());
        vtkNew<vtkPolyDataMapper> mapper;
        vtkNew<vtkPolyDataMapper> mapper1;
        vtkNew<vtkActor> actor;
        vtkNew<vtkActor> actor1;
        actor->SetMapper(mapper.GetPointer());
        actor1->SetMapper(mapper1.GetPointer());
        this->ren->AddActor(actor.GetPointer());
        this->ren->AddActor(actor1.GetPointer());
        this->ren->AutomaticLightCreationOff();
        this->ren->RemoveAllLights();
        vtkNew<vtkCubeSource> ss;
        vtkNew<vtkSphereSource> ss1;
        ss1->SetRadius(1.5);
        ss1->SetCenter(2,0,1);
        mapper->SetInputConnection(ss->GetOutputPort());
        mapper1->SetInputConnection(ss1->GetOutputPort());
//        std::cout << this->renWin->GetSize()[0] << "," << this->renWin->GetSize()[1] << std::endl;
	}

void HelloVrui::toolCreationCallback(Vrui::ToolManager::ToolCreationCallbackData* cbData)
	{

	}

void HelloVrui::frame(void)
	{
        this->renWin->SetSize(const_cast<int*>(Vrui::getWindow(0)->getViewportSize()));
//        std::cout << Vrui::getWindow(0)->getViewportSize()[0] << " " << Vrui::getWindow(0)->getViewportSize()[1] << std::endl;
	}

void HelloVrui::display(GLContextData& contextData) const
	{
	/* Print the modelview and projection matrices: */
	GLdouble mv[16],p[16];
	glGetDoublev(GL_MODELVIEW_MATRIX,mv);
	glGetDoublev(GL_PROJECTION_MATRIX,p);

#if 0 // Printing matrices
        std::cout << "VRUI MV:" << std::endl;
	for(int i=0;i<4;++i)
		{
		for(int j=0;j<4;++j)
			std::cout<<" "<<std::setw(12)<<mv[i+j*4];
                std::cout << std::endl;
                }
                std::cout << std::endl << "VRUI P:" << std::endl;
//		std::cout<<"        ";
	for(int i=0;i<4;++i)
          {
		for(int j=0;j<4;++j)
			std::cout<<" "<<std::setw(12)<<p[i+j*4];
		std::cout<<std::endl;
		}
	std::cout<<std::endl;
#endif

	/* Save OpenGL state: */
	glPushAttrib(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_ENABLE_BIT|GL_LIGHTING_BIT|GL_POLYGON_BIT);
        if(this->drawGLCube)
          {
          drawCube();
          }
        else
          {
//        glMatrixMode(GL_PROJECTION);
//        glLoadIdentity();
//        glPushMatrix();
//        glMatrixMode(GL_MODELVIEW);
//        glPushMatrix();
//        glLoadIdentity();

//        std::cout << this->renWin->GetSize()[0] << "," << this->renWin->GetSize()[1] << std::endl;
//        vtkExternalOpenGLCamera* camera =
//          vtkExternalOpenGLCamera::SafeDownCast(this->ren->GetActiveCamera());
        double p1[16], mv1[16];
        this->transposeMatrix4x4(p,p1);
        this->cam->SetProjectionTransformMatrix(p1);
        this->transposeMatrix4x4(mv,mv1);
        this->cam->SetViewTransformMatrix(mv1);
//        camera->SetFocalPoint(0,0,0);
//        camera->SetPosition(0,0,1.5);
//        std::cout << "Setting position to " << mv[12] << "," << mv[13] << "," << mv[14] << std::endl;
////        camera->SetPosition(0,0,-(mv[14]));
//        camera->SetViewUp(0,1,0);
//        std::cout << "Setting viewup to " << mv[1] << "," << mv[5] << "," << mv[9] << std::endl;
////        camera->SetViewUp(mv[1],mv[5],mv[9]);
//        camera->SetClippingRange(0.001,100);
//        this->renWin->Render();
        glEnable(GL_BLEND);
        glBlendFunc(GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA);
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 15.f);
        renWin->Render();
        glDisable(GL_BLEND);
//        vtkNew<vtkMatrix4x4> mat;
//        mat->DeepCopy(mv1);
//        mat->Invert();
//        vtkLightCollection* lC = this->ren->GetLights();
//        lC->InitTraversal();
//        vtkLight* light = lC->GetNextItem();
//        light->SetLightTypeToSceneLight();
////        light->SetColor(1.0,0.5,0.5);
////        light->SetPosition(10000,0,10000);
//        light->SetTransformMatrix(mat.GetPointer());
////        this->renWin->Render();
////        glPopMatrix();
////        glMatrixMode(GL_PROJECTION);
////        glPopMatrix();

          }
	glPopAttrib();
	}

void HelloVrui::transposeMatrix4x4(double* inArr, double* outArr) const
{
  for(int i = 0 ; i < 4; ++i)
    {
    for (int j = 0; j < 4; ++j)
      {
      outArr[i*4+j] = inArr[j*4+i];
      outArr[j*4+i] = inArr[i*4+j];
      }
    }
}


void HelloVrui::drawCube(void) const
    {
    glBegin(GL_QUADS);		// Draw The Cube Using quads
       glColor3f(0.0f,1.0f,0.0f);	// Color Blue
       glVertex3f( 1.0f, 1.0f,-1.0f);	// Top Right Of The Quad (Top)
       glVertex3f(-1.0f, 1.0f,-1.0f);	// Top Left Of The Quad (Top)
       glVertex3f(-1.0f, 1.0f, 1.0f);	// Bottom Left Of The Quad (Top)
       glVertex3f( 1.0f, 1.0f, 1.0f);	// Bottom Right Of The Quad (Top)
       glColor3f(1.0f,0.5f,0.0f);	// Color Orange
       glVertex3f( 1.0f,-1.0f, 1.0f);	// Top Right Of The Quad (Bottom)
       glVertex3f(-1.0f,-1.0f, 1.0f);	// Top Left Of The Quad (Bottom)
       glVertex3f(-1.0f,-1.0f,-1.0f);	// Bottom Left Of The Quad (Bottom)
       glVertex3f( 1.0f,-1.0f,-1.0f);	// Bottom Right Of The Quad (Bottom)
       glColor3f(1.0f,0.0f,0.0f);	// Color Red
       glVertex3f( 1.0f, 1.0f, 1.0f);	// Top Right Of The Quad (Front)
       glVertex3f(-1.0f, 1.0f, 1.0f);	// Top Left Of The Quad (Front)
       glVertex3f(-1.0f,-1.0f, 1.0f);	// Bottom Left Of The Quad (Front)
       glVertex3f( 1.0f,-1.0f, 1.0f);	// Bottom Right Of The Quad (Front)
       glColor3f(1.0f,1.0f,0.0f);	// Color Yellow
       glVertex3f( 1.0f,-1.0f,-1.0f);	// Top Right Of The Quad (Back)
       glVertex3f(-1.0f,-1.0f,-1.0f);	// Top Left Of The Quad (Back)
       glVertex3f(-1.0f, 1.0f,-1.0f);	// Bottom Left Of The Quad (Back)
       glVertex3f( 1.0f, 1.0f,-1.0f);	// Bottom Right Of The Quad (Back)
       glColor3f(0.0f,0.0f,1.0f);	// Color Blue
       glVertex3f(-1.0f, 1.0f, 1.0f);	// Top Right Of The Quad (Left)
       glVertex3f(-1.0f, 1.0f,-1.0f);	// Top Left Of The Quad (Left)
       glVertex3f(-1.0f,-1.0f,-1.0f);	// Bottom Left Of The Quad (Left)
       glVertex3f(-1.0f,-1.0f, 1.0f);	// Bottom Right Of The Quad (Left)
       glColor3f(1.0f,0.0f,1.0f);	// Color Violet
       glVertex3f( 1.0f, 1.0f,-1.0f);	// Top Right Of The Quad (Right)
       glVertex3f( 1.0f, 1.0f, 1.0f);	// Top Left Of The Quad (Right)
       glVertex3f( 1.0f,-1.0f, 1.0f);	// Bottom Left Of The Quad (Right)
       glVertex3f( 1.0f,-1.0f,-1.0f);	// Bottom Right Of The Quad (Right)
    glEnd();			// End Drawing The Cube
    }

void HelloVrui::menuToggleSelectCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
	{
	/* Adjust program state based on which toggle button changed state: */
	if(strcmp(cbData->toggle->getName(),"ShowSurfaceToggle")==0)
		showSurface=cbData->set;
	else if(strcmp(cbData->toggle->getName(),"SurfaceTransparentToggle")==0)
		surfaceTransparent=cbData->set;
	else if(strcmp(cbData->toggle->getName(),"ShowGridToggle")==0)
		showGrid=cbData->set;
	}

void HelloVrui::centerDisplayCallback(Misc::CallbackData* cbData)
	{
	Vrui::NavTransform nav=Vrui::NavTransform::identity;
	nav*=Vrui::NavTransform::translateFromOriginTo(Vrui::getDisplayCenter());
	nav*=Vrui::NavTransform::rotate(Vrui::Rotation::rotateFromTo(Vrui::Vector(0,0,1),Vrui::getUpDirection()));
	nav*=Vrui::NavTransform::scale(Vrui::Scalar(8)*Vrui::getInchFactor()/Vrui::Scalar(2.0));
	Vrui::setNavigationTransformation(nav);
	}

/* Create and execute an application object: */
/*
 * main - The application main method.
 *
 * parameter argc - int
 * parameter argv - char**
 */
int main(int argc, char* argv[]) {
    try {
        char** applicationDefaults = 0;
        HelloVrui application(argc, argv);
        application.run();
        return 0;
    } catch (std::runtime_error e) {
        std::cerr << "Error: Exception " << e.what() << "!" << std::endl;
        return 1;
    }
}
