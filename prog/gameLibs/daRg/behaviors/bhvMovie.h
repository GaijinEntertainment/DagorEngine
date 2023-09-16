#pragma once

#include <daRg/dag_behavior.h>


namespace darg
{


class BhvMovie : public Behavior
{
public:
  BhvMovie();
  virtual void onAttach(darg::Element *elem) override;
  virtual void onDetach(darg::Element *elem, DetachMode) override;
  virtual void onElemSetup(Element *, darg::SetupMode setup_mode) override;
  virtual int update(UpdateStage, Element *elem, float dt) override;
};


extern BhvMovie bhv_movie;


} // namespace darg
