/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006 Rob Buis <buis@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef SVGMarkerElement_h
#define SVGMarkerElement_h

#include "SVGAnimatedAngle.h"
#include "SVGAnimatedBoolean.h"
#include "SVGAnimatedEnumeration.h"
#include "SVGAnimatedLength.h"
#include "SVGAnimatedPreserveAspectRatio.h"
#include "SVGAnimatedRect.h"
#include "SVGElement.h"
#include "SVGExternalResourcesRequired.h"
#include "SVGFitToViewBox.h"

namespace WebCore {

enum SVGMarkerUnitsType {
    SVGMarkerUnitsUnknown = 0,
    SVGMarkerUnitsUserSpaceOnUse,
    SVGMarkerUnitsStrokeWidth
};

enum SVGMarkerOrientType {
    SVGMarkerOrientUnknown = 0,
    SVGMarkerOrientAuto,
    SVGMarkerOrientAngle,

    // Add new elements before here.
    SVGMarkerOrientMax
};

template<>
struct SVGPropertyTraits<SVGMarkerUnitsType> {
    static unsigned highestEnumValue() { return SVGMarkerUnitsStrokeWidth; }

    static String toString(SVGMarkerUnitsType type)
    {
        switch (type) {
        case SVGMarkerUnitsUnknown:
            return emptyString();
        case SVGMarkerUnitsUserSpaceOnUse:
            return "userSpaceOnUse";
        case SVGMarkerUnitsStrokeWidth:
            return "strokeWidth";
        }

        ASSERT_NOT_REACHED();
        return emptyString();
    }

    static SVGMarkerUnitsType fromString(const String& value)
    {
        if (value == "userSpaceOnUse")
            return SVGMarkerUnitsUserSpaceOnUse;
        if (value == "strokeWidth")
            return SVGMarkerUnitsStrokeWidth;
        return SVGMarkerUnitsUnknown;
    }
};

template<>
struct SVGPropertyTraits<SVGMarkerOrientType> {
    static unsigned highestEnumValue() { return SVGMarkerOrientAngle; }

    // toString is not needed, synchronizeOrientType() handles this on its own.

    static SVGMarkerOrientType fromString(const String& value, SVGAngle& angle)
    {
        if (value == "auto")
            return SVGMarkerOrientAuto;

        ExceptionCode ec = 0;
        angle.setValueAsString(value, ec);
        if (!ec)
            return SVGMarkerOrientAngle;
        return SVGMarkerOrientUnknown;
    }
};

class SVGMarkerElement final : public SVGElement,
                               public SVGExternalResourcesRequired,
                               public SVGFitToViewBox {
public:
    // Forward declare enumerations in the W3C naming scheme, for IDL generation.
    enum {
        SVG_MARKERUNITS_UNKNOWN = SVGMarkerUnitsUnknown,
        SVG_MARKERUNITS_USERSPACEONUSE = SVGMarkerUnitsUserSpaceOnUse,
        SVG_MARKERUNITS_STROKEWIDTH = SVGMarkerUnitsStrokeWidth
    };

    enum {
        SVG_MARKER_ORIENT_UNKNOWN = SVGMarkerOrientUnknown,
        SVG_MARKER_ORIENT_AUTO = SVGMarkerOrientAuto,
        SVG_MARKER_ORIENT_ANGLE = SVGMarkerOrientAngle
    };

    static Ref<SVGMarkerElement> create(const QualifiedName&, Document&);

    AffineTransform viewBoxToViewTransform(float viewWidth, float viewHeight) const;

    void setOrientToAuto();
    void setOrientToAngle(const SVGAngle&);

    static const SVGPropertyInfo* orientTypePropertyInfo();

private:
    SVGMarkerElement(const QualifiedName&, Document&);

    virtual bool needsPendingResourceHandling() const override { return false; }

    static bool isSupportedAttribute(const QualifiedName&);
    virtual void parseAttribute(const QualifiedName&, const AtomicString&) override;
    virtual void svgAttributeChanged(const QualifiedName&) override;
    virtual void childrenChanged(const ChildChange&) override;

    virtual RenderPtr<RenderElement> createElementRenderer(Ref<RenderStyle>&&) override;
    virtual bool rendererIsNeeded(const RenderStyle&) override { return true; }

    virtual bool selfHasRelativeLengths() const override;

    void synchronizeOrientType();

    static const AtomicString& orientTypeIdentifier();
    static const AtomicString& orientAngleIdentifier();
 
    BEGIN_DECLARE_ANIMATED_PROPERTIES(SVGMarkerElement)
        DECLARE_ANIMATED_LENGTH(RefX, refX)
        DECLARE_ANIMATED_LENGTH(RefY, refY)
        DECLARE_ANIMATED_LENGTH(MarkerWidth, markerWidth)
        DECLARE_ANIMATED_LENGTH(MarkerHeight, markerHeight)
        DECLARE_ANIMATED_ENUMERATION(MarkerUnits, markerUnits, SVGMarkerUnitsType)
        DECLARE_ANIMATED_ANGLE(OrientAngle, orientAngle)
        DECLARE_ANIMATED_BOOLEAN(ExternalResourcesRequired, externalResourcesRequired)
        DECLARE_ANIMATED_RECT(ViewBox, viewBox)
        DECLARE_ANIMATED_PRESERVEASPECTRATIO(PreserveAspectRatio, preserveAspectRatio)
    END_DECLARE_ANIMATED_PROPERTIES
  
public:
    // Custom 'orientType' property.
    static void synchronizeOrientType(SVGElement* contextElement);
    static Ref<SVGAnimatedProperty> lookupOrCreateOrientTypeWrapper(SVGElement* contextElement);
    SVGMarkerOrientType& orientType() const;
    SVGMarkerOrientType& orientTypeBaseValue() const { return m_orientType.value; }
    void setOrientTypeBaseValue(const SVGMarkerOrientType& type) { m_orientType.value = type; }
    Ref<SVGAnimatedEnumerationPropertyTearOff<SVGMarkerOrientType>> orientTypeAnimated();

private:
    mutable SVGSynchronizableAnimatedProperty<SVGMarkerOrientType> m_orientType;
};

}

#endif
