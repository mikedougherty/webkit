/*
 * Copyright (C) 2006, 2008, 2009, 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "HTMLViewSourceDocument.h"

#include "Attribute.h"
#include "DOMImplementation.h"
#include "HTMLAnchorElement.h"
#include "HTMLBodyElement.h"
#include "HTMLDivElement.h"
#include "HTMLHtmlElement.h"
#include "HTMLNames.h"
#include "HTMLTableCellElement.h"
#include "HTMLTableElement.h"
#include "HTMLTableRowElement.h"
#include "HTMLTableSectionElement.h"
#include "HTMLDocumentParser.h"
#include "Text.h"
#include "TextDocument.h"

namespace WebCore {

using namespace HTMLNames;

HTMLViewSourceDocument::HTMLViewSourceDocument(Frame* frame, const String& mimeType)
    : HTMLDocument(frame)
    , m_type(mimeType)
{
    setUsesBeforeAfterRules(true);
}

DocumentParser* HTMLViewSourceDocument::createParser()
{
    // Use HTMLDocumentParser if applicable, otherwise use TextDocumentParser.
    if (m_type == "text/html" || m_type == "application/xhtml+xml" || m_type == "image/svg+xml" || DOMImplementation::isXMLMIMEType(m_type)
#if ENABLE(XHTMLMP)
        || m_type == "application/vnd.wap.xhtml+xml"
#endif
        ) {
        // FIXME: Should respect Settings::html5ParserEnabled()
        return new HTMLDocumentParser(this);
    }

    return createTextDocumentParser(this);
}

void HTMLViewSourceDocument::createContainingTable()
{
    RefPtr<HTMLHtmlElement> html = HTMLHtmlElement::create(this);
    addChild(html);
    html->attach();
    RefPtr<HTMLBodyElement> body = HTMLBodyElement::create(this);
    html->addChild(body);
    body->attach();
    
    // Create a line gutter div that can be used to make sure the gutter extends down the height of the whole
    // document.
    RefPtr<HTMLDivElement> div = HTMLDivElement::create(this);
    RefPtr<NamedNodeMap> attrs = NamedNodeMap::create();
    attrs->addAttribute(Attribute::createMapped(classAttr, "webkit-line-gutter-backdrop"));
    div->setAttributeMap(attrs.release());
    body->addChild(div);
    div->attach();

    RefPtr<HTMLTableElement> table = HTMLTableElement::create(this);
    body->addChild(table);
    table->attach();
    m_tbody = HTMLTableSectionElement::create(tbodyTag, this);
    table->addChild(m_tbody);
    m_tbody->attach();
    m_current = m_tbody;
}

void HTMLViewSourceDocument::addViewSourceText(const String& text)
{
    if (!m_current)
        createContainingTable();
    addText(text, "");
}

void HTMLViewSourceDocument::addViewSourceToken(Token* token)
{
    if (!m_current)
        createContainingTable();

    if (token->tagName == textAtom)
        addText(token->text.get(), "");
    else if (token->tagName == commentAtom) {
        if (token->beginTag) {
            m_current = addSpanWithClassName("webkit-html-comment");
            addText(String("<!--") + token->text.get() + "-->", "webkit-html-comment");
        }
    } else {
        // Handle the tag.
        String classNameStr = "webkit-html-tag";
        m_current = addSpanWithClassName(classNameStr);

        String text = "<";
        if (!token->beginTag)
            text += "/";
        text += token->tagName;
        Vector<UChar>* guide = token->m_sourceInfo.get();
        if (!guide || !guide->size())
            text += ">";

        addText(text, classNameStr);

        // Walk our guide string that tells us where attribute names/values should go.
        if (guide && guide->size()) {
            unsigned size = guide->size();
            unsigned begin = 0;
            unsigned currAttr = 0;
            RefPtr<Attribute> attr = 0;
            for (unsigned i = 0; i < size; i++) {
                if (guide->at(i) == 'a' || guide->at(i) == 'x' || guide->at(i) == 'v') {
                    // Add in the string.
                    addText(String(static_cast<UChar*>(guide->data()) + begin, i - begin), classNameStr);
                     
                    begin = i + 1;

                    if (guide->at(i) == 'a') {
                        if (token->attrs && currAttr < token->attrs->length())
                            attr = token->attrs->attributeItem(currAttr++);
                        else
                            attr = 0;
                    }
                    if (attr) {
                        if (guide->at(i) == 'a') {
                            String name = attr->name().toString();
                            
                            m_current = addSpanWithClassName("webkit-html-attribute-name");
                            addText(name, "webkit-html-attribute-name");
                            if (m_current != m_tbody)
                                m_current = static_cast<Element*>(m_current->parent());
                        } else {
                            const String& value = attr->value().string();

                            // Compare ignoring case since HTMLDocumentParser doesn't
                            // lower names when passing in tokens to
                            // HTMLViewSourceDocument.
                            if (equalIgnoringCase(token->tagName, "base") && equalIgnoringCase(attr->name().localName(), "href")) {
                                // Catch the href attribute in the base element.
                                // It will be used for rendering anchors created
                                // by addLink() below.
                                setBaseElementURL(KURL(url(), value));
                            }

                            // FIXME: XML could use namespace prefixes and confuse us.
                            if (equalIgnoringCase(attr->name().localName(), "src") || equalIgnoringCase(attr->name().localName(), "href"))
                                m_current = addLink(value, equalIgnoringCase(token->tagName, "a"));
                            else
                                m_current = addSpanWithClassName("webkit-html-attribute-value");
                            addText(value, "webkit-html-attribute-value");
                            if (m_current != m_tbody)
                                m_current = static_cast<Element*>(m_current->parent());
                        }
                    }
                }
            }
            
            // Add in any string that might be left.
            if (begin < size)
                addText(String(static_cast<UChar*>(guide->data()) + begin, size - begin), classNameStr);

            // Add in the end tag.
            addText(">", classNameStr);
        }
        
        m_current = m_td;
    }
}

void HTMLViewSourceDocument::addViewSourceDoctypeToken(DoctypeToken* doctypeToken)
{
    if (!m_current)
        createContainingTable();
    m_current = addSpanWithClassName("webkit-html-doctype");
    String text = "<";
    text += String::adopt(doctypeToken->m_source);
    text += ">";
    addText(text, "webkit-html-doctype");
}

PassRefPtr<Element> HTMLViewSourceDocument::addSpanWithClassName(const String& className)
{
    if (m_current == m_tbody) {
        addLine(className);
        return m_current;
    }

    RefPtr<HTMLElement> span = HTMLElement::create(spanTag, this);
    RefPtr<NamedNodeMap> attrs = NamedNodeMap::create();
    attrs->addAttribute(Attribute::createMapped(classAttr, className));
    span->setAttributeMap(attrs.release());
    m_current->addChild(span);
    span->attach();
    return span.release();
}

void HTMLViewSourceDocument::addLine(const String& className)
{
    // Create a table row.
    RefPtr<HTMLTableRowElement> trow = HTMLTableRowElement::create(this);
    m_tbody->addChild(trow);
    trow->attach();
    
    // Create a cell that will hold the line number (it is generated in the stylesheet using counters).
    RefPtr<HTMLTableCellElement> td = HTMLTableCellElement::create(tdTag, this);
    RefPtr<NamedNodeMap> attrs = NamedNodeMap::create();
    attrs->addAttribute(Attribute::createMapped(classAttr, "webkit-line-number"));
    td->setAttributeMap(attrs.release());
    trow->addChild(td);
    td->attach();

    // Create a second cell for the line contents
    td = HTMLTableCellElement::create(tdTag, this);
    attrs = NamedNodeMap::create();
    attrs->addAttribute(Attribute::createMapped(classAttr, "webkit-line-content"));
    td->setAttributeMap(attrs.release());
    trow->addChild(td);
    td->attach();
    m_current = m_td = td;

#ifdef DEBUG_LINE_NUMBERS
    RefPtr<Text> lineNumberText = Text::create(this, String::number(parser()->lineNumber() + 1) + " ");
    td->addChild(lineNumberText);
    lineNumberText->attach();
#endif

    // Open up the needed spans.
    if (!className.isEmpty()) {
        if (className == "webkit-html-attribute-name" || className == "webkit-html-attribute-value")
            m_current = addSpanWithClassName("webkit-html-tag");
        m_current = addSpanWithClassName(className);
    }
}

void HTMLViewSourceDocument::addText(const String& text, const String& className)
{
    if (text.isEmpty())
        return;

    // Add in the content, splitting on newlines.
    Vector<String> lines;
    text.split('\n', true, lines);
    unsigned size = lines.size();
    for (unsigned i = 0; i < size; i++) {
        String substring = lines[i];
        if (substring.isEmpty()) {
            if (i == size - 1)
                break;
            substring = " ";
        }
        if (m_current == m_tbody)
            addLine(className);
        RefPtr<Text> t = Text::create(this, substring);
        m_current->addChild(t);
        t->attach();
        if (i < size - 1)
            m_current = m_tbody;
    }
    
    // Set current to m_tbody if the last character was a newline.
    if (text[text.length() - 1] == '\n')
        m_current = m_tbody;
}

PassRefPtr<Element> HTMLViewSourceDocument::addLink(const String& url, bool isAnchor)
{
    if (m_current == m_tbody)
        addLine("webkit-html-tag");
    
    // Now create a link for the attribute value instead of a span.
    RefPtr<HTMLAnchorElement> anchor = HTMLAnchorElement::create(this);
    RefPtr<NamedNodeMap> attrs = NamedNodeMap::create();
    const char* classValue;
    if (isAnchor)
        classValue = "webkit-html-attribute-value webkit-html-external-link";
    else
        classValue = "webkit-html-attribute-value webkit-html-resource-link";
    attrs->addAttribute(Attribute::createMapped(classAttr, classValue));
    attrs->addAttribute(Attribute::createMapped(targetAttr, "_blank"));
    attrs->addAttribute(Attribute::createMapped(hrefAttr, url));
    anchor->setAttributeMap(attrs.release());
    m_current->addChild(anchor);
    anchor->attach();
    return anchor.release();
}

}
