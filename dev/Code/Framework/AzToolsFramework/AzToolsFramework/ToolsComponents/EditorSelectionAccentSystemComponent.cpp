/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include "StdAfx.h"
#include "EditorSelectionAccentSystemComponent.h"

#include <AzCore/Debug/Profiler.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzToolsFramework/API/ComponentEntityObjectBus.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzCore/Slice/SliceComponent.h>

namespace AzToolsFramework
{
    namespace Components
    {
        void EditorSelectionAccentSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serialize->Class<EditorSelectionAccentSystemComponent, AZ::Component>()
                    ->Version(0)
                    ;

                if (AZ::EditContext* ec = serialize->GetEditContext())
                {
                    ec->Class<EditorSelectionAccentSystemComponent>("EditorSelectionAccenting", "Used for selection accenting behavior in the viewport")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ;
                }
            }
        }

        void EditorSelectionAccentSystemComponent::Activate()
        {
            AzToolsFramework::ToolsApplicationEvents::Bus::Handler::BusConnect();
            AzToolsFramework::Components::EditorSelectionAccentingRequestBus::Handler::BusConnect();
        }

        void EditorSelectionAccentSystemComponent::AfterEntityHighlightingChanged()
        {
            if (!m_isAccentRefreshQueued)
            {
                QueueAccentRefresh();
            }
        }

        void EditorSelectionAccentSystemComponent::AfterEntitySelectionChanged(const AzToolsFramework::EntityIdList&, const AzToolsFramework::EntityIdList&)
        {
            if (!m_isAccentRefreshQueued)
            {
                QueueAccentRefresh();
            }
        }

        void EditorSelectionAccentSystemComponent::QueueAccentRefresh()
        {
            AZ_Assert(!m_isAccentRefreshQueued, "Queueing another accent refresh when one is already queued!");
            m_isAccentRefreshQueued = true;

            AZStd::function<void()> accentRefreshCallback =
                [this]()
                {
                    AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::AzToolsFramework, "EditorSelectionAccentSystemComponent::QueueAccentRefresh:AccentRefreshCallback");
                    InvalidateAccents();
                    RecalculateAndApplyAccents();
                    m_isAccentRefreshQueued = false;
                };

            AZ::TickBus::QueueFunction(accentRefreshCallback);
        }

        void EditorSelectionAccentSystemComponent::ForceSelectionAccentRefresh()
        {
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
            InvalidateAccents();
            RecalculateAndApplyAccents();
        }

        void EditorSelectionAccentSystemComponent::InvalidateAccents()
        {
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
            for (const AZ::EntityId& accentedEntity : m_currentlyAccentedEntities)
            {
                AzToolsFramework::ComponentEntityEditorRequestBus::Event(accentedEntity, &AzToolsFramework::ComponentEntityEditorRequests::SetSandboxObjectAccent, ComponentEntityAccentType::None);
            }
        }

        void EditorSelectionAccentSystemComponent::RecalculateAndApplyAccents()
        {
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
            AzToolsFramework::EntityIdList selectedEntities;
            AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(selectedEntities, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);
            AzToolsFramework::EntityIdSet selectedEntitiesSet;
            selectedEntitiesSet.insert(selectedEntities.begin(), selectedEntities.end());

            for (const AZ::EntityId& selectedEntity : selectedEntities)
            {
                // Set selected entities accent to 'Selected'
                AzToolsFramework::ComponentEntityEditorRequestBus::Event(selectedEntity, &AzToolsFramework::ComponentEntityEditorRequests::SetSandboxObjectAccent, ComponentEntityAccentType::Selected);
                m_currentlyAccentedEntities.insert(selectedEntity);

                // Find all selected entities children and Set their accent to 'Parent Selected'
                AzToolsFramework::EntityIdList descendants;
                AZ::TransformBus::EventResult(descendants, selectedEntity, &AZ::TransformInterface::GetAllDescendants);

                for (const AZ::EntityId& descendant : descendants)
                {
                    if (selectedEntitiesSet.find(descendant) == selectedEntitiesSet.end())
                    {
                        AzToolsFramework::ComponentEntityEditorRequestBus::Event(descendant, &ComponentEntityEditorRequests::SetSandboxObjectAccent, ComponentEntityAccentType::ParentSelected);
                        m_currentlyAccentedEntities.insert(descendant);
                    }
                }
            }

            // Find Hovered entities
            // Set their accent to 'Hover'

            AzToolsFramework::EntityIdList highlightedEntities;
            AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(highlightedEntities, &AzToolsFramework::ToolsApplicationRequests::GetHighlightedEntities);

            for (const AZ::EntityId& highlightedEntity : highlightedEntities)
            {
                AzToolsFramework::ComponentEntityEditorRequestBus::Event(highlightedEntity, &ComponentEntityEditorRequests::SetSandboxObjectAccent, ComponentEntityAccentType::Hover);
                m_currentlyAccentedEntities.insert(highlightedEntity);
            }
        }

        void EditorSelectionAccentSystemComponent::Deactivate()
        {
            AzToolsFramework::ToolsApplicationEvents::Bus::Handler::BusConnect();
            AzToolsFramework::Components::EditorSelectionAccentingRequestBus::Handler::BusDisconnect();
        }
    }
} // namespace LmbrCentral
