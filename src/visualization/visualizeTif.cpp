#include <string>
#include <iostream>
#include <vtkSmartPointer.h>
#include <vtkTIFFReader.h>
#include <vtkImageDataGeometryFilter.h>
#include <vtkWarpScalar.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkImageFlip.h>

void visualizeTif(std::string path)
{

  vtkSmartPointer<vtkTIFFReader> reader =
      vtkSmartPointer<vtkTIFFReader>::New();
  reader->SetFileName(path.c_str());
  reader->Update();

  // Flip the image data to correct upside down issue - Iceland appears upside down otherwise
  vtkSmartPointer<vtkImageFlip> flipYFilter =
      vtkSmartPointer<vtkImageFlip>::New();
  flipYFilter->SetInputConnection(reader->GetOutputPort());
  flipYFilter->SetFilteredAxis(1); // Flip along Y axis
  flipYFilter->Update();

  vtkSmartPointer<vtkImageDataGeometryFilter> geometryFilter =
      vtkSmartPointer<vtkImageDataGeometryFilter>::New();
  geometryFilter->SetInputConnection(flipYFilter->GetOutputPort());
  geometryFilter->Update();

  // Adjust warp scalar to correct inverted elevations
  vtkSmartPointer<vtkWarpScalar> warpScalar =
      vtkSmartPointer<vtkWarpScalar>::New();
  warpScalar->SetInputConnection(geometryFilter->GetOutputPort());
  warpScalar->SetScaleFactor(1);
  warpScalar->Update();

  vtkSmartPointer<vtkPolyDataMapper> mapper =
      vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputConnection(warpScalar->GetOutputPort());

  vtkSmartPointer<vtkActor> actor =
      vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);

  vtkSmartPointer<vtkRenderer> renderer =
      vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renderWindow =
      vtkSmartPointer<vtkRenderWindow>::New();
  renderWindow->AddRenderer(renderer);

  vtkSmartPointer<vtkRenderWindowInteractor> interactor =
      vtkSmartPointer<vtkRenderWindowInteractor>::New();
  interactor->SetRenderWindow(renderWindow);

  vtkSmartPointer<vtkInteractorStyleTrackballCamera> style =
      vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
  interactor->SetInteractorStyle(style);

  renderer->AddActor(actor);
  renderer->SetBackground(0.1, 0.2, 0.3);

  renderWindow->Render();
  interactor->Start();
};