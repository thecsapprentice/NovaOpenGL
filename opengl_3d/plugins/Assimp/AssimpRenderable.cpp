#include "../../RenderableManager.h"
#include "../../ApplicationFactory.h"
#include "../../Shader.h"

#include "AssimpRenderable_Model.h"

#include <iostream>
#include <fstream>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Nova {

    class AssimpRenderable : public Renderable {        
        std::unique_ptr<AssimpRenderable_Model> _model;
        bool selected;

    public:
        AssimpRenderable( ApplicationFactory& app ) : Renderable( app ), selected(false)
        {
            _model = std::unique_ptr<AssimpRenderable_Model>( new AssimpRenderable_Model(app) );
        }

        virtual ~AssimpRenderable(){
            
        }
        
        virtual void load(std::string path){
            std::cout << "AssimpRenderable::load" << std::endl;            
            _model->Load_Model( path );
        }


        virtual void draw(){
            glm::mat4 projection,view,model;
            view = _app.GetWorld().Get_ViewMatrix();
            model = _app.GetWorld().Get_ModelMatrix();
            projection = _app.GetWorld().Get_ProjectionMatrix();
            auto _shader = _app.GetShaderManager().GetShader( "BasicMeshShader" );
            _shader->SetMatrix4("projection",projection);
            _shader->SetMatrix4("view",view);
            _shader->SetMatrix4("model",model);
            _shader->SetInteger("selected",selected ? 1 : 0);
            _model->Draw(*(_shader.get()));
        }

        virtual bool selectable() { return true; };

        virtual float hit_test( glm::vec3 start_point, glm::vec3 end_point )
        {
            return _model->TestIntersection(start_point, end_point);
        }

        virtual glm::vec4 bounding_sphere()
        {
            return _model->BoundingSphere();
        }

        virtual void assign_selection( glm::vec3 start_point, glm::vec3 end_point, glm::vec3 intersection )
        {
            selected = true;
        }

        virtual void unassign_selection()
        {
            selected = false;
        }
    };

  
  class AssimpRenderableFactory : public RenderableFactory {
  public:
    AssimpRenderableFactory() : RenderableFactory()
    {}
    
    virtual ~AssimpRenderableFactory() {}

    virtual std::unique_ptr<Renderable> Create( ApplicationFactory& app, std::string path ){
      AssimpRenderable* renderable = new AssimpRenderable(app);
      renderable->load( path );
      return std::unique_ptr<Renderable>( renderable );
    }
    
    virtual bool AcceptExtension(std::string ext) const {
      Assimp::Importer importer;
      return (importer.GetImporter( ext.c_str() ) != NULL);
    };

  };
  

}

extern "C" void registerPlugin(Nova::ApplicationFactory& app) {
  app.GetRenderableManager().AddFactory( std::move( std::unique_ptr<Nova::RenderableFactory>( new Nova::AssimpRenderableFactory() )));
}

extern "C" int getEngineVersion() {
  return Nova::API_VERSION;
}
