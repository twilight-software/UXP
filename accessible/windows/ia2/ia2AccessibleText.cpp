/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ia2AccessibleText.h"

#include "Accessible2.h"
#include "AccessibleText_i.c"

#include "HyperTextAccessibleWrap.h"
#include "HyperTextAccessible-inl.h"
#include "ProxyWrappers.h"
#include "mozilla/ClearOnShutdown.h"

using namespace mozilla::a11y;

StaticRefPtr<HyperTextAccessibleWrap> ia2AccessibleText::sLastTextChangeAcc;
StaticAutoPtr<nsString> ia2AccessibleText::sLastTextChangeString;
uint32_t ia2AccessibleText::sLastTextChangeStart = 0;
uint32_t ia2AccessibleText::sLastTextChangeEnd = 0;
bool ia2AccessibleText::sLastTextChangeWasInsert = false;

// IAccessibleText

STDMETHODIMP
ia2AccessibleText::addSelection(long aStartOffset, long aEndOffset)
{
  A11Y_TRYBLOCK_BEGIN

  MOZ_ASSERT(!HyperTextProxyFor(this));

  HyperTextAccessible* textAcc = static_cast<HyperTextAccessibleWrap*>(this);
  if (textAcc->IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  return textAcc->AddToSelection(aStartOffset, aEndOffset) ?
    S_OK : E_INVALIDARG;

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
ia2AccessibleText::get_attributes(long aOffset, long *aStartOffset,
                                  long *aEndOffset, BSTR *aTextAttributes)
{
  A11Y_TRYBLOCK_BEGIN

  if (!aStartOffset || !aEndOffset || !aTextAttributes)
    return E_INVALIDARG;

  *aStartOffset = 0;
  *aEndOffset = 0;
  *aTextAttributes = nullptr;

  int32_t startOffset = 0, endOffset = 0;
  HRESULT hr;
  MOZ_ASSERT(!HyperTextProxyFor(this));
  HyperTextAccessible* textAcc = static_cast<HyperTextAccessibleWrap*>(this);
  if (textAcc->IsDefunct()) {
    return CO_E_OBJNOTCONNECTED;
  }

  nsCOMPtr<nsIPersistentProperties> attributes =
    textAcc->TextAttributes(true, aOffset, &startOffset, &endOffset);

  hr = AccessibleWrap::ConvertToIA2Attributes(attributes, aTextAttributes);
  if (FAILED(hr))
    return hr;

  *aStartOffset = startOffset;
  *aEndOffset = endOffset;

  return S_OK;

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
ia2AccessibleText::get_caretOffset(long *aOffset)
{
  A11Y_TRYBLOCK_BEGIN

  if (!aOffset)
    return E_INVALIDARG;

  *aOffset = -1;

  MOZ_ASSERT(!HyperTextProxyFor(this));
  HyperTextAccessible* textAcc = static_cast<HyperTextAccessibleWrap*>(this);
  if (textAcc->IsDefunct()) {
    return CO_E_OBJNOTCONNECTED;
  }

  *aOffset = textAcc->CaretOffset();

  return *aOffset != -1 ? S_OK : S_FALSE;

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
ia2AccessibleText::get_characterExtents(long aOffset,
                                        enum IA2CoordinateType aCoordType,
                                        long* aX, long* aY,
                                        long* aWidth, long* aHeight)
{
  A11Y_TRYBLOCK_BEGIN

  if (!aX || !aY || !aWidth || !aHeight)
    return E_INVALIDARG;
  *aX = *aY = *aWidth = *aHeight = 0;

  uint32_t geckoCoordType = (aCoordType == IA2_COORDTYPE_SCREEN_RELATIVE) ?
    nsIAccessibleCoordinateType::COORDTYPE_SCREEN_RELATIVE :
    nsIAccessibleCoordinateType::COORDTYPE_PARENT_RELATIVE;
  nsIntRect rect;
  MOZ_ASSERT(!HyperTextProxyFor(this));
  HyperTextAccessible* textAcc = static_cast<HyperTextAccessibleWrap*>(this);
  if (textAcc->IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  rect = textAcc->CharBounds(aOffset, geckoCoordType);

  *aX = rect.x;
  *aY = rect.y;
  *aWidth = rect.width;
  *aHeight = rect.height;
  return S_OK;

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
ia2AccessibleText::get_nSelections(long* aNSelections)
{
  A11Y_TRYBLOCK_BEGIN

  if (!aNSelections)
    return E_INVALIDARG;
  *aNSelections = 0;

  MOZ_ASSERT(!HyperTextProxyFor(this));
  HyperTextAccessible* textAcc = static_cast<HyperTextAccessibleWrap*>(this);
  if (textAcc->IsDefunct()) {
    return CO_E_OBJNOTCONNECTED;
  }

  *aNSelections = textAcc->SelectionCount();

  return S_OK;

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
ia2AccessibleText::get_offsetAtPoint(long aX, long aY,
                                     enum IA2CoordinateType aCoordType,
                                     long* aOffset)
{
  A11Y_TRYBLOCK_BEGIN

  if (!aOffset)
    return E_INVALIDARG;
  *aOffset = 0;

  uint32_t geckoCoordType = (aCoordType == IA2_COORDTYPE_SCREEN_RELATIVE) ?
    nsIAccessibleCoordinateType::COORDTYPE_SCREEN_RELATIVE :
    nsIAccessibleCoordinateType::COORDTYPE_PARENT_RELATIVE;

  MOZ_ASSERT(!HyperTextProxyFor(this));
  HyperTextAccessible* textAcc = static_cast<HyperTextAccessibleWrap*>(this);
  if (textAcc->IsDefunct()) {
    return CO_E_OBJNOTCONNECTED;
  }

  *aOffset = textAcc->OffsetAtPoint(aX, aY, geckoCoordType);

  return *aOffset == -1 ? S_FALSE : S_OK;

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
ia2AccessibleText::get_selection(long aSelectionIndex, long* aStartOffset,
                                 long* aEndOffset)
{
  A11Y_TRYBLOCK_BEGIN

  if (!aStartOffset || !aEndOffset)
    return E_INVALIDARG;
  *aStartOffset = *aEndOffset = 0;

  int32_t startOffset = 0, endOffset = 0;
  MOZ_ASSERT(!HyperTextProxyFor(this));
  HyperTextAccessible* textAcc = static_cast<HyperTextAccessibleWrap*>(this);
  if (textAcc->IsDefunct()) {
    return CO_E_OBJNOTCONNECTED;
  }

  if (!textAcc->SelectionBoundsAt(aSelectionIndex, &startOffset, &endOffset)) {
    return E_INVALIDARG;
  }

  *aStartOffset = startOffset;
  *aEndOffset = endOffset;
  return S_OK;

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
ia2AccessibleText::get_text(long aStartOffset, long aEndOffset, BSTR* aText)
{
  A11Y_TRYBLOCK_BEGIN

  if (!aText)
    return E_INVALIDARG;

  *aText = nullptr;

  nsAutoString text;
  MOZ_ASSERT(!HyperTextProxyFor(this));
  HyperTextAccessible* textAcc = static_cast<HyperTextAccessibleWrap*>(this);
  if (textAcc->IsDefunct()) {
    return CO_E_OBJNOTCONNECTED;
  }

  if (!textAcc->IsValidRange(aStartOffset, aEndOffset)) {
    return E_INVALIDARG;
  }

  textAcc->TextSubstring(aStartOffset, aEndOffset, text);

  if (text.IsEmpty())
    return S_FALSE;

  *aText = ::SysAllocStringLen(text.get(), text.Length());
  return *aText ? S_OK : E_OUTOFMEMORY;

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
ia2AccessibleText::get_textBeforeOffset(long aOffset,
                                        enum IA2TextBoundaryType aBoundaryType,
                                        long* aStartOffset, long* aEndOffset,
                                        BSTR* aText)
{
  A11Y_TRYBLOCK_BEGIN

  if (!aStartOffset || !aEndOffset || !aText)
    return E_INVALIDARG;

  *aStartOffset = *aEndOffset = 0;
  *aText = nullptr;

  HyperTextAccessible* textAcc = static_cast<HyperTextAccessibleWrap*>(this);
  if (textAcc->IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  if (!textAcc->IsValidOffset(aOffset))
    return E_INVALIDARG;

  nsAutoString text;
  int32_t startOffset = 0, endOffset = 0;

  if (aBoundaryType == IA2_TEXT_BOUNDARY_ALL) {
    startOffset = 0;
    endOffset = textAcc->CharacterCount();
    textAcc->TextSubstring(startOffset, endOffset, text);
  } else {
    AccessibleTextBoundary boundaryType = GetGeckoTextBoundary(aBoundaryType);
    if (boundaryType == -1)
      return S_FALSE;

    textAcc->TextBeforeOffset(aOffset, boundaryType, &startOffset, &endOffset, text);
  }

  *aStartOffset = startOffset;
  *aEndOffset = endOffset;

  if (text.IsEmpty())
    return S_FALSE;

  *aText = ::SysAllocStringLen(text.get(), text.Length());
  return *aText ? S_OK : E_OUTOFMEMORY;

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
ia2AccessibleText::get_textAfterOffset(long aOffset,
                                       enum IA2TextBoundaryType aBoundaryType,
                                       long* aStartOffset, long* aEndOffset,
                                       BSTR* aText)
{
  A11Y_TRYBLOCK_BEGIN

  if (!aStartOffset || !aEndOffset || !aText)
    return E_INVALIDARG;

  *aStartOffset = 0;
  *aEndOffset = 0;
  *aText = nullptr;

  HyperTextAccessible* textAcc = static_cast<HyperTextAccessibleWrap*>(this);
  if (textAcc->IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  if (!textAcc->IsValidOffset(aOffset))
    return E_INVALIDARG;

  nsAutoString text;
  int32_t startOffset = 0, endOffset = 0;

  if (aBoundaryType == IA2_TEXT_BOUNDARY_ALL) {
    startOffset = 0;
    endOffset = textAcc->CharacterCount();
    textAcc->TextSubstring(startOffset, endOffset, text);
  } else {
    AccessibleTextBoundary boundaryType = GetGeckoTextBoundary(aBoundaryType);
    if (boundaryType == -1)
      return S_FALSE;
    textAcc->TextAfterOffset(aOffset, boundaryType, &startOffset, &endOffset, text);
  }

  *aStartOffset = startOffset;
  *aEndOffset = endOffset;

  if (text.IsEmpty())
    return S_FALSE;

  *aText = ::SysAllocStringLen(text.get(), text.Length());
  return *aText ? S_OK : E_OUTOFMEMORY;

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
ia2AccessibleText::get_textAtOffset(long aOffset,
                                    enum IA2TextBoundaryType aBoundaryType,
                                    long* aStartOffset, long* aEndOffset,
                                    BSTR* aText)
{
  A11Y_TRYBLOCK_BEGIN

  if (!aStartOffset || !aEndOffset || !aText)
    return E_INVALIDARG;

  *aStartOffset = *aEndOffset = 0;
  *aText = nullptr;

  HyperTextAccessible* textAcc = static_cast<HyperTextAccessibleWrap*>(this);
  if (textAcc->IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  if (!textAcc->IsValidOffset(aOffset))
    return E_INVALIDARG;

  nsAutoString text;
  int32_t startOffset = 0, endOffset = 0;
  if (aBoundaryType == IA2_TEXT_BOUNDARY_ALL) {
    startOffset = 0;
    endOffset = textAcc->CharacterCount();
    textAcc->TextSubstring(startOffset, endOffset, text);
  } else {
    AccessibleTextBoundary boundaryType = GetGeckoTextBoundary(aBoundaryType);
    if (boundaryType == -1)
      return S_FALSE;
    textAcc->TextAtOffset(aOffset, boundaryType, &startOffset, &endOffset, text);
  }

  *aStartOffset = startOffset;
  *aEndOffset = endOffset;

  if (text.IsEmpty())
    return S_FALSE;

  *aText = ::SysAllocStringLen(text.get(), text.Length());
  return *aText ? S_OK : E_OUTOFMEMORY;

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
ia2AccessibleText::removeSelection(long aSelectionIndex)
{
  A11Y_TRYBLOCK_BEGIN

  MOZ_ASSERT(!HyperTextProxyFor(this));

  HyperTextAccessible* textAcc = static_cast<HyperTextAccessibleWrap*>(this);
  if (textAcc->IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  return textAcc->RemoveFromSelection(aSelectionIndex) ?
    S_OK : E_INVALIDARG;

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
ia2AccessibleText::setCaretOffset(long aOffset)
{
  A11Y_TRYBLOCK_BEGIN

  MOZ_ASSERT(!HyperTextProxyFor(this));

  HyperTextAccessible* textAcc = static_cast<HyperTextAccessibleWrap*>(this);
  if (textAcc->IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  if (!textAcc->IsValidOffset(aOffset))
    return E_INVALIDARG;

  textAcc->SetCaretOffset(aOffset);
  return S_OK;

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
ia2AccessibleText::setSelection(long aSelectionIndex, long aStartOffset,
                                long aEndOffset)
{
  A11Y_TRYBLOCK_BEGIN

  MOZ_ASSERT(!HyperTextProxyFor(this));

  HyperTextAccessible* textAcc = static_cast<HyperTextAccessibleWrap*>(this);
  if (textAcc->IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  return textAcc->SetSelectionBoundsAt(aSelectionIndex, aStartOffset, aEndOffset) ?
    S_OK : E_INVALIDARG;

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
ia2AccessibleText::get_nCharacters(long* aNCharacters)
{
  A11Y_TRYBLOCK_BEGIN

  if (!aNCharacters)
    return E_INVALIDARG;
  *aNCharacters = 0;

  MOZ_ASSERT(!HyperTextProxyFor(this));

  HyperTextAccessible* textAcc = static_cast<HyperTextAccessibleWrap*>(this);
  if (textAcc->IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  *aNCharacters  = textAcc->CharacterCount();
  return S_OK;

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
ia2AccessibleText::scrollSubstringTo(long aStartIndex, long aEndIndex,
                                     enum IA2ScrollType aScrollType)
{
  A11Y_TRYBLOCK_BEGIN

  MOZ_ASSERT(!HyperTextProxyFor(this));

  HyperTextAccessible* textAcc = static_cast<HyperTextAccessibleWrap*>(this);
  if (textAcc->IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  if (!textAcc->IsValidRange(aStartIndex, aEndIndex))
    return E_INVALIDARG;

  textAcc->ScrollSubstringTo(aStartIndex, aEndIndex, aScrollType);
  return S_OK;

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
ia2AccessibleText::scrollSubstringToPoint(long aStartIndex, long aEndIndex,
                                          enum IA2CoordinateType aCoordType,
                                          long aX, long aY)
{
  A11Y_TRYBLOCK_BEGIN

  uint32_t geckoCoordType = (aCoordType == IA2_COORDTYPE_SCREEN_RELATIVE) ?
    nsIAccessibleCoordinateType::COORDTYPE_SCREEN_RELATIVE :
    nsIAccessibleCoordinateType::COORDTYPE_PARENT_RELATIVE;

  MOZ_ASSERT(!HyperTextProxyFor(this));

  HyperTextAccessible* textAcc = static_cast<HyperTextAccessibleWrap*>(this);
  if (textAcc->IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  if (!textAcc->IsValidRange(aStartIndex, aEndIndex))
    return E_INVALIDARG;

  textAcc->ScrollSubstringToPoint(aStartIndex, aEndIndex,
                                  geckoCoordType, aX, aY);
  return S_OK;

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
ia2AccessibleText::get_newText(IA2TextSegment *aNewText)
{
  A11Y_TRYBLOCK_BEGIN

  return GetModifiedText(true, aNewText);

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
ia2AccessibleText::get_oldText(IA2TextSegment *aOldText)
{
  A11Y_TRYBLOCK_BEGIN

  return GetModifiedText(false, aOldText);

  A11Y_TRYBLOCK_END
}

// ia2AccessibleText

HRESULT
ia2AccessibleText::GetModifiedText(bool aGetInsertedText,
                                   IA2TextSegment *aText)
{
  if (!aText)
    return E_INVALIDARG;

  if (!sLastTextChangeAcc)
    return S_OK;

  if (aGetInsertedText != sLastTextChangeWasInsert)
    return S_OK;

  if (sLastTextChangeAcc != this)
    return S_OK;

  aText->start = sLastTextChangeStart;
  aText->end = sLastTextChangeEnd;

  if (sLastTextChangeString->IsEmpty())
    return S_FALSE;

  aText->text = ::SysAllocStringLen(sLastTextChangeString->get(), sLastTextChangeString->Length());
  return aText->text ? S_OK : E_OUTOFMEMORY;
}

AccessibleTextBoundary
ia2AccessibleText::GetGeckoTextBoundary(enum IA2TextBoundaryType aBoundaryType)
{
  switch (aBoundaryType) {
    case IA2_TEXT_BOUNDARY_CHAR:
      return nsIAccessibleText::BOUNDARY_CHAR;
    case IA2_TEXT_BOUNDARY_WORD:
      return nsIAccessibleText::BOUNDARY_WORD_START;
    case IA2_TEXT_BOUNDARY_LINE:
      return nsIAccessibleText::BOUNDARY_LINE_START;
    //case IA2_TEXT_BOUNDARY_SENTENCE:
    //case IA2_TEXT_BOUNDARY_PARAGRAPH:
      // XXX: not implemented
    default:
      return -1;
  }
}

void
ia2AccessibleText::InitTextChangeData()
{
  ClearOnShutdown(&sLastTextChangeAcc);
  ClearOnShutdown(&sLastTextChangeString);
}

void
ia2AccessibleText::UpdateTextChangeData(HyperTextAccessibleWrap* aAcc,
                                        bool aInsert, const nsString& aStr,
                                        int32_t aStart, uint32_t aLen)
{
  if (!sLastTextChangeString)
    sLastTextChangeString = new nsString();

  sLastTextChangeAcc = aAcc;
  sLastTextChangeStart = aStart;
  sLastTextChangeEnd = aStart + aLen;
  sLastTextChangeWasInsert = aInsert;
  *sLastTextChangeString = aStr;
}
