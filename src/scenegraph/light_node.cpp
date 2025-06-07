#include "light_node.h"

#include "render_server.h"

PTLightNode_T::PTLightNode_T(PTDeserialiser::ArgMap arguments) : PTNode_T(arguments)
{
	getArg(arguments, "colour", colour);
	getArg(arguments, "brightness", brightness);
	getArg(arguments, "half_angle", half_angle);
	if (hasArg(arguments, "directional", PTDeserialiser::ArgType::INT_ARG))
		directional = arguments["directional"].i_val > 0;

	PTRenderServer::get()->addLight(this);
}

PTLightNode_T::~PTLightNode_T()
{
	PTRenderServer::get()->removeLight(this);
}

LightDescription PTLightNode_T::getDescription()
{
	LightDescription desc{ };
	desc.colour = colour;
	desc.is_directional = directional ? 1.0f : 0.0f;
	desc.multiplier = brightness;
	desc.position = getTransform()->getPosition();
	desc.cos_half_ang_radians = cos((float)(half_angle * (M_PI / 180.0f)));
	desc.direction = -getTransform()->getForward();

	return desc;
}
