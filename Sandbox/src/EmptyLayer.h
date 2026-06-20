#pragma once

#include <Vulkitten.h>
#include "imgui.h"

class EmptyLayer : public Vulkitten::Layer
{
public:
    EmptyLayer() : Layer("Empty")
    {
    }

    void OnUpdate(Vulkitten::Timestep timestep, Vulkitten::SceneContext& ctx) override
    {
    }

    virtual void OnImguiRender() override
    {
	}

    void OnEvent(Vulkitten::Event& event) override
    {
    }

private:
};