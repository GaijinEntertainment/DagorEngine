from "%darg/ui_imports.nut" import *

return {
  rendObj = ROBJ_FRAME
  size = [500, SIZE_TO_CONTENT]
  hplace = ALIGN_CENTER
  vplace = ALIGN_CENTER
  borderWidth = 1
  children = [
    {
      rendObj = ROBJ_TEXTAREA
      size = [flex(), SIZE_TO_CONTENT]
      behavior = Behaviors.TextArea
      text = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Fusce nec leo eu ante pulvinar efficitur at ut dui. Cras sit amet turpis nulla. Etiam sit amet ante at tortor pellentesque posuere. Fusce leo sapien, tincidunt et nunc eget, fringilla lobortis justo. Cras et orci et ex gravida consequat non at eros. Fusce sit amet tincidunt sapien. Maecenas vitae odio in dui pellentesque feugiat. Etiam aliquet libero volutpat, blandit enim sit amet, mattis nunc. Vivamus velit dui, efficitur ut dignissim sollicitudin, pulvinar nec libero. Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas. Integer laoreet consequat sodales. Pellentesque sed ipsum risus. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia curae; Etiam dapibus eleifend nulla. Duis at porttitor dolor, in congue velit."
    }
/*
Expected behavior: total 25 gap between columns,
because there is no horizintal flow to apply margin,
but padding should be applied anyway

Current behavior: positive offset 15 from the top and
negative offset -15 by horizontal position

Margin should not be applied for siblings that not
distributed by flow attribute of the parent container.

Padding should be applied everytime, keeping external
container size and adding gaps betweeen bounding box
and internal elements.
*/
    {
      rendObj = ROBJ_TEXTAREA
      size = [200, SIZE_TO_CONTENT]
      pos = [200, 0]
      padding = 25
      margin = 15
      behavior = Behaviors.TextArea
      hplace = ALIGN_RIGHT
      text = "Integer sagittis risus at interdum hendrerit. Fusce nec vehicula velit. Pellentesque ut massa et erat tempor auctor. Proin at faucibus magna, a blandit urna. Sed dictum orci a mi tincidunt, vel ultrices dui tempus. Etiam fringilla non enim vel posuere. Quisque fermentum tincidunt turpis ac ullamcorper. Curabitur id orci porttitor, hendrerit diam vitae, malesuada ante. Donec mattis tincidunt augue, vulputate convallis mauris fringilla quis. Suspendisse feugiat enim sit amet erat dapibus dictum."
    }
    {
      rendObj = ROBJ_IMAGE
      size = [100, 100]
      pos = [-100, 0]
      padding = 25
      margin = 15
      keepAspect = KEEP_ASPECT_FILL
      image = Picture("ui/ca_cup1")
    }
  ]
}
