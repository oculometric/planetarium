#pragma once

#include "node.h"

class PTMesh;

class PTTextNode : public PTNode
{
	friend class PTResourceManager;
private:
	struct TextBuffer
	{
		PTVector2f fontmap_glyph_size = PTVector2f{ 10.0f, 18.0f };
		float aspect_ratio = 1.0f;
		uint32_t characters_per_line = 16;
		PTVector4u text[64] = { PTVector4u{ 0x20202020, 0x20202020, 0x20202020, 0x20202020 } };
	};

private:
	std::string text = "Hello, World!";
	TextBuffer uniform_buffer;
	PTMesh* mesh = nullptr;
	PTMaterial* material = nullptr;

public:
	void setText(std::string _text);
	inline std::string getText() const { return text; }

	inline void setLineWidth(uint32_t width) { uniform_buffer.characters_per_line = width; updateUniforms(); }
	inline uint32_t getLineWidth() const { return uniform_buffer.characters_per_line; }

	void updateUniforms();

protected:
	PTTextNode(PTDeserialiser::ArgMap arguments);
	~PTTextNode();
};