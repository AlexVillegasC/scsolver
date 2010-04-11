/****************************************************************************
 * Copyright (c) 2005-2009 Kohei Yoshida
 * 
 * This code is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3 only,
 * as published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License version 3 for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * version 3 along with this work.  If not, see
 * <http://www.gnu.org/licenses/>.
 ****************************************************************************/

#ifndef _SCSOLVER_NUMERIC_HOOKEJEEVES_HXX_
#define _SCSOLVER_NUMERIC_HOOKEJEEVES_HXX_

#include "numeric/nlpbase.hxx"
#include <vector>

namespace scsolver { namespace numeric { namespace nlp {

class HookeJeeves : public BaseAlgorithm
{
public:
    explicit HookeJeeves();
    virtual ~HookeJeeves();

    virtual void solve();

private:
    size_t m_maxIteration;
    bool m_debug;
};

}}}

#endif
